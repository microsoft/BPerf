// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using Microsoft.Diagnostics.Symbols;

namespace Microsoft.BPerf.StackViewer
{
    using System;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.AspNetCore.Http;
    using Microsoft.BPerf.StackViewer.ConfigurationBinders;
    using Microsoft.BPerf.SymbolicInformation.ProgramDatabase;
    using Microsoft.BPerf.SymbolServer.Interfaces;
    using Microsoft.Extensions.Caching.Memory;
    using Microsoft.Extensions.Configuration;
    using Microsoft.Extensions.Options;
    using Microsoft.QuickInject;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Serialization;

    public sealed class Startup
    {
        private readonly JsonSerializerSettings jsonSerializerSettings = new JsonSerializerSettings { ContractResolver = new CamelCasePropertyNamesContractResolver() };

        private QuickInjectContainer dependencyInjectionContainer;

        private string contentRoot;

        private string indexFile;

        public void SetupQuickInjectContainer(QuickInjectContainer container)
        {
            this.dependencyInjectionContainer = container;
        }

        public void Configure(IApplicationBuilder app, IHostingEnvironment env)
        {
            this.contentRoot = Path.Combine(env.ContentRootPath, "spa", "build");
            this.indexFile = Path.GetFullPath(Path.Join(this.contentRoot, "index.html"));

            var container = this.dependencyInjectionContainer;

            var configuration = new ConfigurationBuilder().SetBasePath(env.ContentRootPath).AddJsonFile("appsettings.json", optional: false, reloadOnChange: false).Build();
            container.RegisterInstance<IConfiguration>(configuration);
            container.RegisterInstance(new SymbolReader(new EventSourceTextWriter()));
            container.RegisterTypeAsResolutionContext<HttpContext>();

            container.RegisterType<IOptions<SymbolServerInformation>, SymbolServerInformationConfig>(new ContainerControlledLifetimeManager());
            container.RegisterType<IOptions<CacheSettings>, CacheSettingsConfig>(new ContainerControlledLifetimeManager());
            container.RegisterType<IOptions<StackViewerSettings>, StackViewerSettingsConfig>(new ContainerControlledLifetimeManager());
            container.RegisterType<IOptions<SourceServerAuthorizationInformation>, SourceServerAuthorizationInformationConfig>(new ContainerControlledLifetimeManager());
            container.RegisterType<IOptions<MemoryCacheOptions>, MemoryCacheOptionsConfig>(new ContainerControlledLifetimeManager());
            container.RegisterType<IDeserializedDataCache, DeserializedDataCache>(new ContainerControlledLifetimeManager());
            container.RegisterType<TextWriter, EventSourceTextWriter>(new ContainerControlledLifetimeManager());
            container.RegisterType<CallTreeDataCache, CallTreeDataCache>(new ContainerControlledLifetimeManager());
            container.RegisterType<ISymbolServerInformation, SymbolServerInformationProvider>(new ContainerControlledLifetimeManager());
            container.RegisterType<ISourceServerAuthorizationInformationProvider, SourceServerAuthorizationInformationProvider>(new ContainerControlledLifetimeManager());
            container.RegisterType<ICacheExpirationTimeProvider, CacheExpirationTimeProvider>(new ContainerControlledLifetimeManager());

            container.RegisterType<ISymbolServerArtifactRetriever, SymbolServerArtifactRetriever>();
            container.RegisterType<StackViewerModel, StackViewerModel>();
            container.RegisterType<ICallTreeData, CallTreeData>();

            // TODO: Make this crossplat
            var etlDir = Path.GetFullPath(Environment.ExpandEnvironmentVariables(container.Resolve<IOptions<StackViewerSettings>>().Value.TemporaryDataFileDownloadLocation));
            container.Resolve<IOptions<StackViewerSettings>>().Value.TemporaryDataFileDownloadLocation = etlDir;
            Directory.CreateDirectory(etlDir);
        }

        public async Task HandleRequest(HttpContext context)
        {
            string requestPath = context.Request.Path.Value;

            if (requestPath.StartsWith("/api"))
            {
                if (requestPath.StartsWith(@"/api/eventdata"))
                {
                    var controller = this.dependencyInjectionContainer.Resolve<EventViewerController>(context);
                    await WriteJsonResponse(context, this.jsonSerializerSettings, await controller.EventsAPI());
                }
                else
                {
                    var controller = this.dependencyInjectionContainer.Resolve<StackViewerController>(context);
                    context.Response.ContentType = "application/json; charset=UTF-8";

                    if (requestPath.StartsWith(@"/api/callerchildren"))
                    {
                        var name = (string)context.Request.Query["name"] ?? string.Empty;
                        var path = (string)context.Request.Query["path"] ?? string.Empty;

                        await WriteJsonResponse(context, this.jsonSerializerSettings, await controller.CallerChildrenAPI(name, path));
                    }
                    else if (requestPath.StartsWith(@"/api/treenode"))
                    {
                        var name = (string)context.Request.Query["name"] ?? string.Empty;
                        await WriteJsonResponse(context, this.jsonSerializerSettings, await controller.TreeNodeAPI(name));
                    }
                    else if (requestPath.StartsWith(@"/api/hotspots"))
                    {
                        await WriteJsonResponse(context, this.jsonSerializerSettings, await controller.HotspotsAPI());
                    }
                    else if (requestPath.StartsWith(@"/api/eventlist"))
                    {
                        await WriteJsonResponse(context, this.jsonSerializerSettings, await controller.EventListAPI());
                    }
                    else if (requestPath.StartsWith(@"/api/processlist"))
                    {
                        await WriteJsonResponse(context, this.jsonSerializerSettings, await controller.ProcessListAPI());
                    }
                    else if (requestPath.StartsWith(@"/api/drillinto"))
                    {
                        bool exclusive = requestPath.StartsWith(@"/api/drillinto/exclusive");

                        var name = (string)context.Request.Query["name"] ?? string.Empty;
                        var path = (string)context.Request.Query["path"] ?? string.Empty;

                        await WriteJsonResponse(context, this.jsonSerializerSettings, await controller.DrillIntoAPI(exclusive, name, path));
                    }
                    else if (requestPath.StartsWith(@"/api/lookupwarmsymbols"))
                    {
                        await WriteJsonResponse(context, this.jsonSerializerSettings, await controller.LookupWarmSymbolsAPI(1));
                    }
                }
            }
            else if (requestPath.StartsWith("/ui"))
            {
                await SendIndexFile(context, this.indexFile);
            }
            else
            {
                var fullPath = Path.GetFullPath(Path.Join(this.contentRoot.AsSpan(), requestPath.AsSpan(1)));
                if (fullPath.StartsWith(this.contentRoot) && File.Exists(fullPath))
                {
                    var ext = Path.GetExtension(fullPath);
                    if (ext.EndsWith("js"))
                    {
                        context.Response.ContentType = "application/javascript; charset=UTF-8";
                    }
                    else if (ext.EndsWith("css"))
                    {
                        context.Response.ContentType = "text/css; charset=UTF-8";
                    }
                    else if (ext.EndsWith("html"))
                    {
                        context.Response.ContentType = "text/html; charset=UTF-8";
                    }

                    await context.Response.SendFileAsync(fullPath);
                }
                else
                {
                    if (requestPath.Equals("/"))
                    {
                        await SendIndexFile(context, this.indexFile);
                    }
                    else
                    {
                        context.Response.StatusCode = 404;
                        await context.Response.WriteAsync("404 Not Found");
                    }
                }
            }
        }

        private static async Task SendIndexFile(HttpContext context, string indexFile)
        {
            context.Response.ContentType = "text/html; charset=UTF-8";
            await context.Response.SendFileAsync(indexFile);
        }

        private static async Task WriteJsonResponse(HttpContext context, JsonSerializerSettings settings, object data)
        {
            await context.Response.WriteAsync(JsonConvert.SerializeObject(data, settings));
        }
    }
}

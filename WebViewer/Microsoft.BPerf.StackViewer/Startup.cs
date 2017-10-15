// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System;
    using System.IO;
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.AspNetCore.Http;
    using Microsoft.BPerf.SymbolicInformation.Abstractions;
    using Microsoft.BPerf.SymbolicInformation.ProgramDatabase;
    using Microsoft.Extensions.Configuration;
    using Microsoft.Extensions.DependencyInjection;
    using Microsoft.Extensions.Logging;
    using Microsoft.Extensions.Options;

    public sealed class Startup
    {
        public Startup(IHostingEnvironment env)
        {
            var builder = new ConfigurationBuilder()
                .SetBasePath(env.ContentRootPath)
                .AddJsonFile("appsettings.json", optional: false, reloadOnChange: true)
                .AddJsonFile($"appsettings.{env.EnvironmentName}.json", optional: true)
                .AddEnvironmentVariables();
            this.Configuration = builder.Build();
        }

        public IConfigurationRoot Configuration { get; }

        public void ConfigureServices(IServiceCollection services)
        {
            services.Configure<SymbolServerInformation>(this.Configuration.GetSection("SymbolServerInformation"));
            services.Configure<CacheSettings>(this.Configuration.GetSection("CacheSettings"));
            services.Configure<StackViewerSettings>(this.Configuration.GetSection("StackViewerSettings"));

            services.AddMvc();
            services.AddMemoryCache();
            services.AddTransient<StackViewerController, StackViewerController>();
            services.AddTransient<ICallTreeData, CallTreeData>();
            services.AddTransient<ISymbolServerArtifactRetriever, SymbolServerArtifactRetriever>();
            services.AddTransient<StackViewerModel, StackViewerModel>();
            services.AddSingleton<IDeserializedDataCache, DeserializedDataCache>();
            services.AddSingleton<CallTreeDataCache, CallTreeDataCache>();
            services.AddSingleton<ISymbolServerInformation, SymbolServerInformationProvider>();
            services.AddSingleton<IHttpContextAccessor, HttpContextAccessor>();
            services.AddSingleton<ICacheExpirationTimeProvider, CacheExpirationTimeProvider>();
            services.AddSingleton<TextWriter, EventSourceTextWriter>();
        }

        public void Configure(IApplicationBuilder app, IHostingEnvironment env, ILoggerFactory loggerFactory)
        {
            // TODO: Make this crossplat
            var etlDir = Environment.ExpandEnvironmentVariables(app.ApplicationServices.GetService<IOptions<StackViewerSettings>>().Value.TemporaryDataFileDownloadLocation);
            Directory.CreateDirectory(etlDir);
            Directory.SetCurrentDirectory(etlDir);

            loggerFactory.AddConsole();
            app.UseStaticFiles();
            app.UseMvc();
        }
    }
}

// Copyright (c) .NET Foundation. All rights reserved.
// Licensed under the MIT Licensee. See LICENSE in the project root for license information.

namespace BPerfViewer
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Net.Sockets;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Hosting.Server;
    using Microsoft.AspNetCore.Hosting.Server.Features;
    using Microsoft.AspNetCore.Http.Features;

    public sealed class TcpListenerServer : IServer
    {
        public TcpListenerServer()
        {
            this.Features = new FeatureCollection();
            this.Features.Set<IServerAddressesFeature>(new ServerAddressesFeature());
        }

        public void Dispose()
        {
        }

        public void Start<TContext>(IHttpApplication<TContext> application)
        {
            if (application == null)
            {
                throw new ArgumentNullException(nameof(application));
            }

            Task.Factory.StartNew(() => Action(application));
        }

        private async void Action<TContext>(IHttpApplication<TContext> application)
        {
            var context = default(TContext);
            var applicationException = default(Exception);

            var serverAddressFeature = this.Features.Get<IServerAddressesFeature>();
            if (serverAddressFeature.Addresses.Count > 1 || serverAddressFeature.Addresses.Count < 1)
            {
                throw new Exception("TcpListener only supports one server address");
            }
            
            string address = serverAddressFeature.Addresses.First();

            int indexOfFirstColon = address.IndexOf(':') + 3;
            int indexOfLastColon = address.LastIndexOf(':');

            var host = address.Substring(indexOfFirstColon, indexOfLastColon - indexOfFirstColon);
            if (string.Equals(host, "localhost", StringComparison.OrdinalIgnoreCase))
            {
                host = "127.0.0.1";
            }

            int port = int.Parse(address.Substring(address.LastIndexOf(':') + 1));
            var listener = new TcpListener(IPAddress.Parse(host), port);
            listener.Start();
            while (true)
            {
                var tcpClient = await listener.AcceptTcpClientAsync();
                var tt = Task.Factory.StartNew(async () =>
                {
                    try
                    {
                        var stream = tcpClient.GetStream();
                        var request = new TcpListenerHttpRequest(stream);
                        var memStream = new MemoryStream();
                        var response = new TcpListenerHttpResponse(memStream);

                        var featureCollection = new FeatureCollection();
                        featureCollection.Set<IHttpRequestFeature>(request);
                        featureCollection.Set<IHttpResponseFeature>(response);

                        context = application.CreateContext(featureCollection);
                        await application.ProcessRequestAsync(context);

                        using (var sw = new StreamWriter(stream))
                        {
                            sw.Write($"HTTP/1.1 {response.StatusCode} {response.ReasonPhrase}");
                            sw.Write('\r');
                            sw.Write('\n');

                            foreach (var header in response.Headers)
                            {
                                sw.Write(header.Key);
                                sw.Write(':');
                                sw.Write(' ');
                                sw.Write((string)header.Value);
                                sw.Write('\r');
                                sw.Write('\n');
                            }

                            sw.Write('\r');
                            sw.Write('\n');

                            sw.Flush();
                            sw.BaseStream.Flush();

                            var arr = memStream.ToArray();
                            sw.BaseStream.Write(arr, 0, arr.Length);
                            sw.Flush();
                        }
                    }
                    catch (Exception e)
                    {
                        applicationException = e;
                    }
                    finally
                    {
                        application.DisposeContext(context, applicationException);
                    }
                });
            }
        }

        private static void CopyStream(Stream input, Stream output)
        {
            byte[] buffer = new byte[32768];
            int read;
            while ((read = input.Read(buffer, 0, buffer.Length)) > 0)
            {
                output.Write(buffer, 0, read);
            }
        }

        public IFeatureCollection Features { get; }
    }
}
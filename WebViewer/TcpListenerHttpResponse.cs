namespace BPerfViewer
{
    using System;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Http;
    using Microsoft.AspNetCore.Http.Features;

    public sealed class TcpListenerHttpResponse : IHttpResponseFeature
    {
        public TcpListenerHttpResponse(Stream outgoing)
        {
            this.Body = outgoing;
            this.Headers = new HeaderDictionary();
            this.StatusCode = 200;
            this.ReasonPhrase = "OK";
        }

        public Stream Body { get; set; }

        public bool HasStarted { get; }

        public IHeaderDictionary Headers { get; set; }

        public string ReasonPhrase { get; set; }

        public int StatusCode { get; set; }

        public void OnCompleted(Func<object, Task> callback, object state)
        {
        }

        public void OnStarting(Func<object, Task> callback, object state)
        {
        }
    }
}
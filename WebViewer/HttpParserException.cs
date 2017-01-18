namespace BPerfViewer
{
    using System;

    public class HttpParserException : Exception
    {
        public HttpParserException(string message) : base(message)
        {
        }
    }
}
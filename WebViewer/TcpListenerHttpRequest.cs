namespace BPerfViewer
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Runtime.CompilerServices;
    using Microsoft.AspNetCore.Http;
    using Microsoft.AspNetCore.Http.Features;
    using Microsoft.Extensions.Primitives;

    public sealed class TcpListenerHttpRequest : IHttpRequestFeature
    {
        public TcpListenerHttpRequest(Stream incoming)
        {
            var line = ReadLine(incoming);
            var tokens = line.Split(' '); // tuple candidate

            if (tokens.Length != 3)
            {
                ThrowHttpParserException("Unable to find Method, Path and Protocol in first line of request");
            }

            this.Method = tokens[0];
            this.Path = tokens[1];
            this.Protocol = tokens[2];

            this.RawTarget = this.Path;

            int queryStringIndexOf = this.Path.IndexOf('?');
            this.QueryString = queryStringIndexOf == -1 ? string.Empty : this.Path.Substring(queryStringIndexOf);

            this.FillUp(incoming);
        }

        public string Protocol { get; set; }

        public string Scheme
        {
            get { return "http"; }
            set
            {
                if (!string.Equals(value, "http", StringComparison.OrdinalIgnoreCase))
                {
                    throw new NotSupportedException("TcpListener does not support a Scheme other than 'http'");
                }
            }
        }

        public string Method { get; set; }

        public string PathBase
        {
            get { return string.Empty; }
            set
            {
                if (!string.Equals(value, string.Empty, StringComparison.Ordinal))
                {
                    throw new NotSupportedException("TcpListener does not support setting a custom PathBase");
                }
            }
        }

        public string Path { get; set; }

        public string QueryString { get; set; }

        public string RawTarget { get; set; }

        public IHeaderDictionary Headers { get; set; }

        public Stream Body { get; set; }

        private void FillUp(Stream stream)
        {
            this.Headers = new HeaderDictionary();

            string line;
            while ((line = ReadLine(stream)) != null)
            {
                if (string.Equals(line, string.Empty))
                {
                    break;
                }

                int index = line.IndexOf(':');
                if (index == -1 || index == 0 || index == line.Length - 1)
                {
                    ThrowHttpParserException($"Unable to parse header line ${line}");
                }

                int pos = index + 1;
                while ((pos < line.Length) && line[pos] == ' ')
                {
                    ++pos;
                }

                this.Headers.Add(line.Substring(0, index), line.Substring(pos, line.Length - pos));
            }

            StringValues contentLengthString;
            if (this.Headers.TryGetValue("Content-Length", out contentLengthString))
            {
                int contentLength = int.Parse(contentLengthString);
                var buf = new byte[contentLength];

                for (int i = 0; i < contentLength;)
                {
                    i += stream.Read(buf, i, contentLength);
                }

                this.Body = new MemoryStream(buf);
            }
            else
            {
                this.Body = Stream.Null;
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void ThrowHttpParserException(string message)
        {
            throw new HttpParserException(message);
        }

        private static string ReadLine(Stream stream)
        {
            var charList = new List<char>(128); // pre-allocate a reasonable value

            while (true)
            {
                var @byte = stream.ReadByte();

                if (@byte == '\n') // right, we should really be looking at \r\n together
                {
                    break;
                }

                if (@byte == '\r')
                {
                    continue;
                }

                charList.Add((char)@byte);
            }

            return new string(charList.ToArray());
        }
    }
}
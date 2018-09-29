namespace Microsoft.BPerf.StackViewer
{
    using System.Collections.Generic;
    using Microsoft.AspNetCore.Http;

    public class ProcessListViewModel : StackViewerModel
    {
        public ProcessListViewModel(HttpContext httpContext)
            : base(httpContext)
        {
        }

        public List<ProcessInfo> ProcessList { get; set; }
    }
}

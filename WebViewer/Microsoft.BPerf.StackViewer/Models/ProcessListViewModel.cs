namespace Microsoft.BPerf.StackViewer
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;
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

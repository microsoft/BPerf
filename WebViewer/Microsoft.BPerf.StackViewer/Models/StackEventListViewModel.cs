namespace Microsoft.BPerf.StackViewer
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Http;

    public class StackEventListViewModel : StackViewerModel
    {
        public StackEventListViewModel(HttpContext httpContext)
            : base(httpContext)
        {
        }

        public List<StackEventTypeInfo> StackEventTypeList { get; set; }
    }
}

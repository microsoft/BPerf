namespace Microsoft.BPerf.StackViewer.Controllers
{
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Mvc;

    public sealed class HomeController : Controller
    {
        private readonly StackViewerModel model;

        private readonly IDeserializedDataCache dataCache;

        public HomeController(IDeserializedDataCache dataCache, StackViewerModel model)
        {
            this.dataCache = dataCache;
            this.model = model;
        }

        [Route("/", Name = "Index")]
        public ActionResult Index()
        {
            return this.View();
        }

        [Route("/clearcache", Name = "ClearCache")]
        public ActionResult ClearCache()
        {
            this.dataCache.ClearAllCacheEntries();
            return new RedirectResult("/");
        }

        [Route("/ui/eventlist")]
        public async ValueTask<ActionResult> StackEventTypeList()
        {
            var deserializedData = this.dataCache.GetData(this.model);

            return this.View(new StackEventListViewModel(this.HttpContext)
            {
                StackEventTypeList = await deserializedData.GetStackEventTypesAsync()
            });
        }

        [Route("/eventlist")]
        public async ValueTask<List<StackEventTypeInfo>> StackEventTypeListJson()
        {
            var deserializedData = this.dataCache.GetData(this.model);
            return await deserializedData.GetStackEventTypesAsync();
        }

        [Route("/ui/processlist")]
        public async ValueTask<ActionResult> ProcessList()
        {
            var deserializedData = this.dataCache.GetData(this.model);

            return this.View(new ProcessListViewModel(this.HttpContext)
            {
                ProcessList = await deserializedData.GetProcessListAsync()
            });
        }

        [Route("/processlist")]
        public async ValueTask<List<ProcessInfo>> ProcessListJson()
        {
            var deserializedData = this.dataCache.GetData(this.model);
            return await deserializedData.GetProcessListAsync();
        }
    }
}

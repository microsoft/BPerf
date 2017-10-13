// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Mvc;
    using Microsoft.Extensions.Options;

    public sealed class StackViewerUIController : Controller
    {
        private readonly StackViewerController controller;

        private readonly StackViewerSettings stackViewerSettings;

        public StackViewerUIController(StackViewerController controller, IOptions<StackViewerSettings> stackViewerSettings)
        {
            if (controller == null)
            {
                ThrowHelper.ThrowArgumentNullException(nameof(controller));
            }

            this.controller = controller;
            this.stackViewerSettings = stackViewerSettings.Value;
        }

        [Route("ui/stackviewer/summary", Name = "HotSpots")]
        public async ValueTask<ActionResult> Hotspots()
        {
            var model = new StackViewerViewModel(this.HttpContext)
            {
                TreeNodes = await this.controller.Hotspots(this.stackViewerSettings.NumberOfDisplayedTableEntries)
            };

            this.ViewBag.Title = "Hotspots Viewer";
            return this.View(model);
        }

        [Route("ui/stackviewer/callertree", Name = "Callers", Order = 2)]
        public async ValueTask<ActionResult> Callers(string name)
        {
            if (name.Contains("&"))
            {
                name = name.Replace("&", "%26");
            }

            this.ViewBag.Title = "Callers Viewer";

            var model = new CallersViewStackViewerViewModel(this.HttpContext)
            {
                Node = await this.controller.Node(name),
                TreeNodes = await this.controller.CallerTree(name)
            };

            return this.View(model);
        }

        [Route("ui/stackviewer/callertree/children", Name = "CallersChildren", Order = 1)]
        public async ValueTask<ActionResult> CallersChildren(string name, string path)
        {
            if (name.Contains("&"))
            {
                name = name.Replace("&", "%26");
            }

            var model = new CallersViewStackViewerViewModel(this.HttpContext)
            {
                TreeNodes = await this.controller.CallerTree(name, path)
            };

            this.ViewBag.Title = "Callers Viewer";
            return this.View(model);
        }

        [Route("ui/stackviewer/source/callertree", Name = "SourceViewer")]
        public async ValueTask<ActionResult> SourceViewer(string name, string path)
        {
            if (name.Contains("&"))
            {
                name = name.Replace("&", "%26");
            }

            return this.View(await this.controller.CallerContextSource(name, path));
        }
    }
}

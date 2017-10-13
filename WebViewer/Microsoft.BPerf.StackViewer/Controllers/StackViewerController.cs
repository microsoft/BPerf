// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Mvc;

    public sealed class StackViewerController
    {
        private readonly IDeserializedDataCache dataCache;

        private readonly StackViewerModel model;

        public StackViewerController(IDeserializedDataCache dataCache, StackViewerModel model)
        {
            this.dataCache = dataCache;
            this.model = model;
        }

        [HttpGet]
        [Route("stackviewer/resolvesymbol", Name = "ResolveSymbol")]
        public ContentResult ResolveSymbol(uint pid, string eip)
        {
            // return new ContentResult { Content = this.data.ResolveSymbol(pid, ulong.Parse(eip, System.Globalization.NumberStyles.HexNumber)) };
            return null;
        }

        [HttpGet]
        [Route("stackviewer/summary")]
        public async ValueTask<List<TreeNode>> Hotspots(int numNodes)
        {
            var data = await this.GetData();
            return await data.GetSummaryTree(numNodes);
        }

        [HttpGet]
        [Route("stackviewer/node")]
        public async ValueTask<TreeNode> Node(string name)
        {
            if (name == null)
            {
                return null;
            }

            var data = await this.GetData();
            return await data.GetNode(name);
        }

        [HttpGet]
        [Route("stackviewer/callertree")]
        public async ValueTask<TreeNode[]> CallerTree(string name, string path = "")
        {
            if (name == null)
            {
                return null;
            }

            var data = await this.GetData();
            return await data.GetCallerTree(name, path);
        }

        [HttpGet]
        [Route("stackviewer/calleetree")]
        public async ValueTask<TreeNode[]> CalleeTree(string name, string path = "")
        {
            if (name == null)
            {
                return null;
            }

            var data = await this.GetData();
            return await data.GetCalleeTree(name, path);
        }

        [HttpGet]
        [Route("stackviewer/source")]
        public async ValueTask<SourceInformation> Source(string name)
        {
            if (name == null)
            {
                return null;
            }

            var data = await this.GetData();
            return await data.Source(await data.GetNode(name));
        }

        [HttpGet]
        [Route("stackviewer/source/caller")]
        public async ValueTask<SourceInformation> CallerContextSource(string name, string path = "")
        {
            if (name == null)
            {
                return null;
            }

            var data = await this.GetData();
            return await data.Source(await data.GetCallerTreeNode(name, path));
        }

        [HttpGet]
        [Route("stackviewer/source/callee")]
        public async ValueTask<SourceInformation> CalleeContextSource(string name, string path = "")
        {
            if (name == null)
            {
                return null;
            }

            var data = await this.GetData();
            return await data.Source(await data.GetCalleeTreeNode(name, path));
        }

        private ValueTask<ICallTreeData> GetData()
        {
            var deserializedData = this.dataCache.GetData(this.model);
            return deserializedData.GetCallTreeAsync(this.model);
        }
    }
}

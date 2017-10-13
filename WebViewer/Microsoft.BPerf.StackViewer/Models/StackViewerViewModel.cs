// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System.Collections.Generic;
    using Microsoft.AspNetCore.Http;

    public class StackViewerViewModel : StackViewerModel
    {
        public StackViewerViewModel(HttpContext httpContext)
            : base(httpContext)
        {
        }

        public IEnumerable<TreeNode> TreeNodes { get; set; }
    }
}
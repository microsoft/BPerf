// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using Microsoft.AspNetCore.Http;

    public sealed class CallersViewStackViewerViewModel : StackViewerViewModel
    {
        public CallersViewStackViewerViewModel(HttpContext httpContext)
            : base(httpContext)
        {
        }

        public TreeNode Node { get; set; }
    }
}
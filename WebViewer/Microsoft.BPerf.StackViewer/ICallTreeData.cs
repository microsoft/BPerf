// Copyright (c) MiValueTask<crosoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System.Collections.Generic;
    using System.Threading.Tasks;

    public interface ICallTreeData
    {
        // gets a node without a context
        ValueTask<TreeNode> GetNode(string name);

        // gets a node with a caller tree context and looks up the path
        ValueTask<TreeNode> GetCallerTreeNode(string name, string path = "");

        // gets a node with a callee tree context and looks up the path
        ValueTask<TreeNode> GetCalleeTreeNode(string name, string path = "");

        // gets a list of nodes with no context
        ValueTask<List<TreeNode>> GetSummaryTree(int numNodes);

        // returns a flat caller tree given a node
        ValueTask<TreeNode[]> GetCallerTree(string name);

        // returns a flat caller tree given a node and its context
        ValueTask<TreeNode[]> GetCallerTree(string name, string path);

        // returns a flat callee tree given a node
        ValueTask<TreeNode[]> GetCalleeTree(string name);

        // returns a flat callee tree given a node and its context
        ValueTask<TreeNode[]> GetCalleeTree(string name, string path);

        ValueTask<SourceInformation> Source(TreeNode node);
    }
}

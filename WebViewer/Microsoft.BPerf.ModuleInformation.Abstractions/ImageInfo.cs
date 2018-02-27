// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.ModuleInformation.Abstractions
{
    using System;
    using System.Collections.Generic;
    using System.IO;

    public sealed class ImageInfo : SearchableInfo, IComparable<ImageInfo>
    {
        public ImageInfo(ulong begin, ulong end, string filePath)
        {
            this.Begin = begin;
            this.End = end;
            this.FilePath = filePath.ToLowerInvariant();
            this.ImageName = Path.GetFileNameWithoutExtension(filePath); // TODO: Remove System.IO.*
            this.InstructionPointers = new HashSet<ulong>();
        }

        public ImageInfo(ulong begin)
        {
            this.Begin = begin;
        }

        public static ImageInfo EmptyImageInfo { get; } = new ImageInfo(0, 0, string.Empty);

        public override ulong Begin { get; }

        public override ulong End { get; set; }

        public string FilePath { get; }

        public string ImageName { get; }

        public HashSet<ulong> InstructionPointers { get; }

        public int CompareTo(ImageInfo other)
        {
            if (this.Begin == other.Begin)
            {
                return 0;
            }

            return this.Begin < other.Begin ? -1 : 1;
        }
    }
}

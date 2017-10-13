// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackInformation.Abstractions
{
    using System.Collections.Generic;

    public static class Extensions
    {
        public static int RangeBinarySearch<T>(this List<T> sortedArray, ulong frame)
            where T : SearchableInfo
        {
            return RangeBinarySearch(sortedArray, 0, sortedArray.Count - 1, frame);
        }

        private static int RangeBinarySearch<T>(List<T> sortedArray, int first, int last, ulong frame)
            where T : SearchableInfo
        {
            if (first <= last)
            {
                int mid = (first + last) / 2;

                // if the frame is imageBase or within its size
                if (frame >= sortedArray[mid].Begin && frame <= sortedArray[mid].End)
                {
                    return mid;
                }

                if (frame < sortedArray[mid].Begin)
                {
                    return RangeBinarySearch(sortedArray, first, mid - 1, frame);
                }

                return RangeBinarySearch(sortedArray, mid + 1, last, frame);
            }

            return -1;
        }
    }
}

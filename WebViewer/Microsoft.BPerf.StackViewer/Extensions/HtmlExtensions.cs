// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.BPerf.StackViewer
{
    using System;
    using Microsoft.AspNetCore.Html;
    using Microsoft.AspNetCore.Mvc;
    using Microsoft.AspNetCore.Mvc.Rendering;
    using Microsoft.AspNetCore.Mvc.Routing;

    public static class HtmlExtensions
    {
        public static string Json(this IHtmlHelper html, object obj)
        {
            return Newtonsoft.Json.JsonConvert.SerializeObject(obj);
        }

        public static HtmlString CustomRouteLink(this IHtmlHelper htmlHelper, string linkText, string routeName, object routeValues)
        {
            var request = htmlHelper.ViewContext.HttpContext.Request;
            UrlHelper urlHelper = new UrlHelper(htmlHelper.ViewContext);
            UriBuilder uriBuilder = new UriBuilder(urlHelper.RouteUrl(routeName, routeValues, request.Scheme));

            var str = uriBuilder.ToString();

            foreach (var p in request.Query)
            {
                if (string.Equals(p.Key, "path", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                str = Microsoft.AspNetCore.WebUtilities.QueryHelpers.AddQueryString(str, p.Key, p.Value);
            }

            return new HtmlString("<a href=\"" + str + "\">" + linkText + "</a>");
        }

        public static HtmlString RouteLinkTargetTab(this IHtmlHelper htmlHelper, string linkText, string routeName, object routeValues)
        {
            var request = htmlHelper.ViewContext.HttpContext.Request;
            UrlHelper urlHelper = new UrlHelper(htmlHelper.ViewContext);
            UriBuilder uriBuilder = new UriBuilder(urlHelper.RouteUrl(routeName, routeValues, request.Scheme));

            var str = uriBuilder.ToString();

            foreach (var p in request.Query)
            {
                if (string.Equals(p.Key, "path", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                str = Microsoft.AspNetCore.WebUtilities.QueryHelpers.AddQueryString(str, p.Key, p.Value);
            }

            return new HtmlString("<a target=\"_blank\" href=\"" + str + "\">" + linkText + "</a>");
        }

        public static string MyRouteLinkAjax(this IHtmlHelper htmlHelper)
        {
            return htmlHelper.ViewContext.HttpContext.Request.QueryString.ToString().Substring(1);
        }
    }
}
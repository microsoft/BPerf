namespace Microsoft.BPerf.SymbolicInformation.Abstractions
{
    using System;
    using System.Threading.Tasks;

    public interface ITracePdbSymbolReaderProvider
    {
        ValueTask<IPdbSymbolReader> GetSymbolReader(string peFilePath);

        ValueTask<IPdbSymbolReader> GetSymbolReader(string peFilePath, Guid signature, uint age, string pdbName);

        IPdbSymbolReader GetSymbolReader(Guid signature, uint age, string pdbName);
    }
}

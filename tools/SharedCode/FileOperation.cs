using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Ribbon.Shared
{
    class FileOperation
    {
        public static void SlideDataFile(string[] fileNames, string folder)
        {
            foreach (var fileName in fileNames)
            {
                try
                {
                    File.Delete(folder + Constants.old3prefix + fileName);
                }
                catch (Exception) { }
                try
                {
                    File.Move(folder + Constants.old2prefix + fileName, folder + Constants.old3prefix + fileName);
                }
                catch (Exception) { }
                try
                {
                    File.Move(folder + Constants.old1prefix + fileName, folder + Constants.old2prefix + fileName);
                }
                catch (Exception) { }
                try
                {
                    File.Move(folder + fileName, folder + Constants.old1prefix + fileName);
                }
                catch (Exception) { }
                try
                {
                    File.Delete(folder + fileName);// just in case
                }
                catch (Exception) { }
            }
        }
    }
}

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

        public static void RunPostProcessor()
        {
            var process = System.Diagnostics.Process.GetCurrentProcess(); // Or whatever method you are using
            string fullPath = process.MainModule.FileName;
            var folder = Path.GetDirectoryName(fullPath);
            var postProdcessor = Path.Combine(folder, Constants.postProcessor);

            System.Diagnostics.ProcessStartInfo processStart = new System.Diagnostics.ProcessStartInfo(postProdcessor);
            processStart.CreateNoWindow = true;
            processStart.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;

            System.Diagnostics.Process p = System.Diagnostics.Process.Start(processStart);
            // p.WaitForExit();
        }

        public static void Upload()
        {
            var process = System.Diagnostics.Process.GetCurrentProcess(); // Or whatever method you are using
            string fullPath = process.MainModule.FileName;
            var folder = Path.GetDirectoryName(fullPath);
            var ftpUploader = Path.Combine(folder, Constants.ftpUploader);

            System.Diagnostics.ProcessStartInfo processStart = new System.Diagnostics.ProcessStartInfo("cmd.exe", "/C " + ftpUploader);
            processStart.CreateNoWindow = true;
            processStart.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;

            System.Diagnostics.Process p = System.Diagnostics.Process.Start(processStart);
            p.WaitForExit();
        }

    }
}

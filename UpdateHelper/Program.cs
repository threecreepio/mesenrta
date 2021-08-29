using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Diagnostics;
using System.Windows.Forms;
using System.IO.Compression;

namespace MesenUpdater
{
	class Program
	{
		static void Main(string[] args)
		{
			if(args.Length > 1) {
				string srcFile = args[0];
				string destDir = args[1];
				string processPath = Path.Combine(destDir, "Mesen.exe");

				//Wait a bit for the application to shut down before trying to kill it
				System.Threading.Thread.Sleep(1000);
				try {
					foreach(Process process in Process.GetProcessesByName("Mesen")) {
						try {
							if (process.MainModule.FileName == processPath)
							{
								process.Kill();
							}
						} catch { }
					}
				} catch { }

				int retryCount = 0;
				while(retryCount < 10) {
					try {
						using(FileStream file = File.Open(processPath, FileMode.Open, FileAccess.ReadWrite, FileShare.Delete | FileShare.ReadWrite)) { }
						break;
					} catch {
						retryCount++;
						System.Threading.Thread.Sleep(1000);
					}
				}

				try
				{
					using (var zip = ZipFile.OpenRead(srcFile))
					{
						foreach (var entry in zip.Entries)
						{
							entry.ExtractToFile(Path.Combine(destDir, entry.FullName), true);
						}
					}
				} catch {
					MessageBox.Show("Update failed. Please try downloading and installing the new version manually.", "Mesen", MessageBoxButtons.OK, MessageBoxIcon.Error);
					return;
				}
				Process.Start(Path.Combine(destDir, "Mesen.exe"));
			} else {
				MessageBox.Show("Please run Mesen directly to update.", "Mesen");
			}
		}
	}
}

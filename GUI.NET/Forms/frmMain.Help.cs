using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Xml;
using Mesen.GUI.Config;
using Newtonsoft.Json.Linq;

namespace Mesen.GUI.Forms
{
	public partial class frmMain
	{
		private void CheckForUpdates(bool displayResult)
		{
			Task.Run(() => {
				try {
					using(var client = new WebClient()) {
						client.Headers.Add("User-Agent", "Mesen Updater");
						var response = client.DownloadString("https://api.github.com/repos/threecreepio/mesenrta/releases/latest");
						dynamic obj = Newtonsoft.Json.JsonConvert.DeserializeObject(response);
						var currentVersion = new Version(InteropEmu.GetMesenVersion());
						var latestVersion = new Version((string) obj.name);
						var changeLog = ((string) obj.body) ?? "";
						var downloadUrl = ((JArray)obj.assets)
							.Select(asset => asset["browser_download_url"])
							.Where(url => url != null && url.ToString().EndsWith(".zip"))
							.Select(url => url.ToString())
							.FirstOrDefault();

						if (latestVersion > currentVersion)
						{
								this.BeginInvoke((MethodInvoker)(() =>
								{
									using (frmUpdatePrompt frmUpdate = new frmUpdatePrompt(currentVersion, latestVersion, changeLog, downloadUrl))
									{
										if (frmUpdate.ShowDialog(null, this) == DialogResult.OK)
										{
											Close();
										}
									}
								}));
						}
						else if (displayResult)
						{
							MesenMsgBox.Show("MesenUpToDate", MessageBoxButtons.OK, MessageBoxIcon.Information);
						}
					}
				} catch(Exception ex) {
					if(displayResult) {
						MesenMsgBox.Show("ErrorWhileCheckingUpdates", MessageBoxButtons.OK, MessageBoxIcon.Error, ex.ToString());
					}
				}
			});
		}

		private void mnuAbout_Click(object sender, EventArgs e)
		{
			using(frmAbout frm = new frmAbout()) {
				frm.ShowDialog(this);
			}
		}

		private void mnuCheckForUpdates_Click(object sender, EventArgs e)
		{
			CheckForUpdates(true);
		}
		
		private void mnuOnlineHelp_Click(object sender, EventArgs e)
		{
			string platform = Program.IsMono ? "linux" : "win";
			Process.Start("http://www.mesen.ca/docs/?v=" + InteropEmu.GetMesenVersion() + "&p=" + platform + "&l=" + ResourceHelper.GetLanguageCode());
		}

		private void mnuReportBug_Click(object sender, EventArgs e)
		{
			Process.Start("http://www.mesen.ca/ReportBug.php");
		}

		private void mnuHelpWindow_Click(object sender, EventArgs e)
		{
			using(frmHelp frm = new frmHelp()) {
				frm.ShowDialog(sender, this);
			}
		}
	}
}

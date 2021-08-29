using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Mesen.GUI.Config;
using Mesen.GUI.Controls;

namespace Mesen.GUI.Forms
{
	public partial class frmUpdatePrompt : BaseForm
	{
		private string _downloadUrl;

		public frmUpdatePrompt(Version currentVersion, Version latestVersion, string changeLog, string downloadUrl)
		{
			InitializeComponent();

			this.txtChangelog.Font = new System.Drawing.Font(BaseControl.MonospaceFontFamily, 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));

			_downloadUrl = downloadUrl;

			lblCurrentVersionString.Text = currentVersion.ToString();
			lblLatestVersionString.Text = latestVersion.ToString();
			txtChangelog.Text = changeLog.Replace("\n", Environment.NewLine);
		}

		protected override void OnLoad(EventArgs e)
		{
			base.OnLoad(e);

			btnUpdate.Focus();
		}
		
		private void btnUpdate_Click(object sender, EventArgs e)
		{
#if DISABLEAUTOUPDATE
			MesenMsgBox.Show("AutoUpdateDisabledMessage", MessageBoxButtons.OK, MessageBoxIcon.Information);
			this.DialogResult = DialogResult.Cancel;
			this.Close();
#else
			string destFilePath = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);
			string srcFilePath = Path.Combine(ConfigManager.DownloadFolder, "Mesen." + lblLatestVersionString.Text + ".zip");
			//string backupFilePath = Path.Combine(ConfigManager.BackupFolder, "Mesen." + lblCurrentVersionString.Text + ".exe");
			string updateHelper = Path.Combine(ConfigManager.HomeFolder, "MesenUpdater.exe");

			if(!File.Exists(updateHelper)) {
				MesenMsgBox.Show("UpdaterNotFound", MessageBoxButtons.OK, MessageBoxIcon.Error);
				DialogResult = DialogResult.Cancel;
			} else if(!string.IsNullOrWhiteSpace(srcFilePath)) {
				frmDownloadProgress frmDownload = new frmDownloadProgress(_downloadUrl, srcFilePath);
				if(frmDownload.ShowDialog() == DialogResult.OK) {
					FileInfo fileInfo = new FileInfo(srcFilePath);
					if(fileInfo.Length > 0) {
						if(Program.IsMono) {
							Process.Start("mono", string.Format("\"{0}\" \"{1}\" \"{2}\"", updateHelper, srcFilePath, destFilePath));
						} else {
							Process.Start(updateHelper, string.Format("\"{0}\" \"{1}\"", srcFilePath, destFilePath));
						}
					} else {
						//Download failed, mismatching hashes
						MesenMsgBox.Show("UpdateDownloadFailed", MessageBoxButtons.OK, MessageBoxIcon.Error);
						DialogResult = DialogResult.Cancel;
					}
				}
			}
#endif
		}
	}
}

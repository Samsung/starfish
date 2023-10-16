namespace StarfishWinformShell
{
    partial class BrowserUIForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.buttonNavigate = new System.Windows.Forms.Button();
            this.textBoxAddress = new System.Windows.Forms.TextBox();
            this.buttonConsole = new System.Windows.Forms.Button();
            this.buttonResizeFHDHalf = new System.Windows.Forms.Button();
            this.buttonResizeFHD = new System.Windows.Forms.Button();
            this.textBoxFocusReceiver = new System.Windows.Forms.TextBox();
            this.panelBrowserContent = new System.Windows.Forms.Panel();
            this.buttonSettings = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // buttonNavigate
            // 
            this.buttonNavigate.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonNavigate.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.buttonNavigate.Location = new System.Drawing.Point(904, 0);
            this.buttonNavigate.Name = "buttonNavigate";
            this.buttonNavigate.Size = new System.Drawing.Size(66, 22);
            this.buttonNavigate.TabIndex = 0;
            this.buttonNavigate.Text = "Navigate";
            this.buttonNavigate.UseVisualStyleBackColor = true;
            this.buttonNavigate.Click += new System.EventHandler(this.buttonNavigate_Click);
            // 
            // textBoxAddress
            // 
            this.textBoxAddress.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxAddress.Location = new System.Drawing.Point(1, 1);
            this.textBoxAddress.Name = "textBoxAddress";
            this.textBoxAddress.Size = new System.Drawing.Size(899, 21);
            this.textBoxAddress.TabIndex = 1;
            this.textBoxAddress.Text = "about:blank";
            this.textBoxAddress.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textBoxAddress_KeyDown);
            this.textBoxAddress.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBoxAddress_KeyPress);
            this.textBoxAddress.KeyUp += new System.Windows.Forms.KeyEventHandler(this.textBoxAddress_KeyUp);
            // 
            // buttonConsole
            // 
            this.buttonConsole.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonConsole.Location = new System.Drawing.Point(976, 0);
            this.buttonConsole.Name = "buttonConsole";
            this.buttonConsole.Size = new System.Drawing.Size(63, 22);
            this.buttonConsole.TabIndex = 3;
            this.buttonConsole.Text = "Console";
            this.buttonConsole.UseVisualStyleBackColor = true;
            this.buttonConsole.Click += new System.EventHandler(this.buttonConsole_Click);
            // 
            // buttonResizeFHDHalf
            // 
            this.buttonResizeFHDHalf.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonResizeFHDHalf.Location = new System.Drawing.Point(1093, 1);
            this.buttonResizeFHDHalf.Name = "buttonResizeFHDHalf";
            this.buttonResizeFHDHalf.Size = new System.Drawing.Size(120, 22);
            this.buttonResizeFHDHalf.TabIndex = 4;
            this.buttonResizeFHDHalf.Text = "Resize to FHD-.5x";
            this.buttonResizeFHDHalf.UseVisualStyleBackColor = true;
            this.buttonResizeFHDHalf.Visible = false;
            this.buttonResizeFHDHalf.Click += new System.EventHandler(this.buttonResizeFHDHalf_Click);
            // 
            // buttonResizeFHD
            // 
            this.buttonResizeFHD.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonResizeFHD.Location = new System.Drawing.Point(1110, 0);
            this.buttonResizeFHD.Name = "buttonResizeFHD";
            this.buttonResizeFHD.Size = new System.Drawing.Size(102, 22);
            this.buttonResizeFHD.TabIndex = 5;
            this.buttonResizeFHD.Text = "Resize to FHD";
            this.buttonResizeFHD.UseVisualStyleBackColor = true;
            this.buttonResizeFHD.Click += new System.EventHandler(this.buttonResizeFHD_Click);
            // 
            // textBoxFocusReceiver
            // 
            this.textBoxFocusReceiver.Location = new System.Drawing.Point(1088, 437);
            this.textBoxFocusReceiver.Name = "textBoxFocusReceiver";
            this.textBoxFocusReceiver.Size = new System.Drawing.Size(100, 21);
            this.textBoxFocusReceiver.TabIndex = 6;
            this.textBoxFocusReceiver.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textBoxFocusReceiver_KeyDown);
            this.textBoxFocusReceiver.KeyUp += new System.Windows.Forms.KeyEventHandler(this.textBoxFocusReceiver_KeyUp);
            // 
            // panelBrowserContent
            // 
            this.panelBrowserContent.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panelBrowserContent.Location = new System.Drawing.Point(1, 25);
            this.panelBrowserContent.Name = "panelBrowserContent";
            this.panelBrowserContent.Size = new System.Drawing.Size(1211, 819);
            this.panelBrowserContent.TabIndex = 7;
            this.panelBrowserContent.Click += new System.EventHandler(this.panelBrowserContent_Click);
            this.panelBrowserContent.MouseDown += new System.Windows.Forms.MouseEventHandler(this.panelBrowserContent_MouseDown);
            this.panelBrowserContent.MouseEnter += new System.EventHandler(this.panelBrowserContent_MouseEnter);
            this.panelBrowserContent.MouseLeave += new System.EventHandler(this.panelBrowserContent_MouseLeave);
            this.panelBrowserContent.MouseMove += new System.Windows.Forms.MouseEventHandler(this.panelBrowserContent_MouseMove);
            this.panelBrowserContent.MouseUp += new System.Windows.Forms.MouseEventHandler(this.panelBrowserContent_MouseUp);
            // 
            // buttonSettings
            // 
            this.buttonSettings.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonSettings.Location = new System.Drawing.Point(1044, 0);
            this.buttonSettings.Name = "buttonSettings";
            this.buttonSettings.Size = new System.Drawing.Size(63, 22);
            this.buttonSettings.TabIndex = 8;
            this.buttonSettings.Text = "Settings";
            this.buttonSettings.UseVisualStyleBackColor = true;
            this.buttonSettings.Click += new System.EventHandler(this.buttonSettings_Click);
            // 
            // BrowserUIForm
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlLightLight;
            this.ClientSize = new System.Drawing.Size(1215, 845);
            this.Controls.Add(this.panelBrowserContent);
            this.Controls.Add(this.buttonResizeFHD);
            this.Controls.Add(this.buttonConsole);
            this.Controls.Add(this.textBoxAddress);
            this.Controls.Add(this.buttonNavigate);
            this.Controls.Add(this.textBoxFocusReceiver);
            this.Controls.Add(this.buttonSettings);
            this.Controls.Add(this.buttonResizeFHDHalf);
            this.DoubleBuffered = true;
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Name = "BrowserUIForm";
            this.Text = "StarFish Windows Shell";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.BrowserUIForm_FormClosed);
            this.Load += new System.EventHandler(this.BrowserUIForm_Load);
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.BrowserUIForm_DragDrop);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.BrowserUIForm_DragEnter);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.BrowserUIForm_KeyDown);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.BrowserUIForm_KeyUp);
            this.Resize += new System.EventHandler(this.BrowserUIForm_Resize);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button buttonNavigate;
        private System.Windows.Forms.TextBox textBoxAddress;
        private System.Windows.Forms.Button buttonConsole;
        private System.Windows.Forms.Button buttonResizeFHDHalf;
        private System.Windows.Forms.Button buttonResizeFHD;
        private System.Windows.Forms.TextBox textBoxFocusReceiver;
        private System.Windows.Forms.Panel panelBrowserContent;
        private System.Windows.Forms.Button buttonSettings;
    }
}
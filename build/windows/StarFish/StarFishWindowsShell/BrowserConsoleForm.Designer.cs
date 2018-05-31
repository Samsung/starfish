namespace StarFishWindowsShell
{
    partial class Console
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
            this.textBoxConsoleInput = new System.Windows.Forms.TextBox();
            this.webBrowserConsoleOutput = new System.Windows.Forms.WebBrowser();
            this.SuspendLayout();
            // 
            // textBoxConsoleInput
            // 
            this.textBoxConsoleInput.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxConsoleInput.Location = new System.Drawing.Point(0, 430);
            this.textBoxConsoleInput.Name = "textBoxConsoleInput";
            this.textBoxConsoleInput.Size = new System.Drawing.Size(800, 21);
            this.textBoxConsoleInput.TabIndex = 0;
            this.textBoxConsoleInput.TextChanged += new System.EventHandler(this.textBoxConsoleInput_TextChanged);
            this.textBoxConsoleInput.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBoxConsoleInput_KeyPress);
            this.textBoxConsoleInput.KeyUp += new System.Windows.Forms.KeyEventHandler(this.textBoxConsoleInput_KeyUp);
            // 
            // webBrowserConsoleOutput
            // 
            this.webBrowserConsoleOutput.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.webBrowserConsoleOutput.Location = new System.Drawing.Point(0, 1);
            this.webBrowserConsoleOutput.MinimumSize = new System.Drawing.Size(20, 20);
            this.webBrowserConsoleOutput.Name = "webBrowserConsoleOutput";
            this.webBrowserConsoleOutput.Size = new System.Drawing.Size(800, 423);
            this.webBrowserConsoleOutput.TabIndex = 1;
            // 
            // Console
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.webBrowserConsoleOutput);
            this.Controls.Add(this.textBoxConsoleInput);
            this.Name = "Console";
            this.Text = "BrowserConsoleForm";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox textBoxConsoleInput;
        private System.Windows.Forms.WebBrowser webBrowserConsoleOutput;
    }
}
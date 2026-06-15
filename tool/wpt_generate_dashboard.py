#!/usr/bin/env python3
# Copyright (c) 2026-present Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Generate WPT status dashboard HTML from cumulative data.json.

Creates an index.html dashboard with Chart.js graphs showing:
- Pass rate trend over time
- Passed/Failed test count trend
- List of recent reports with links

Uses Chart.js CDN to avoid external dependencies. Data is inlined as a
JavaScript variable to work around GitHub Pages CORS restrictions.

    python3 tool/wpt_generate_dashboard.py \\
      --data-file data.json \\
      --output index.html
"""

import json
import os
import sys
from argparse import ArgumentParser
from html import escape


def load_data(path):
    """Load cumulative data.json; return list of metrics dicts."""
    if not os.path.isfile(path):
        return []
    try:
        with open(path) as fp:
            data = json.load(fp)
        # Handle both list (default) and dict formats gracefully
        if isinstance(data, dict):
            return data.get("history", [])
        return data if isinstance(data, list) else []
    except (json.JSONDecodeError, IOError):
        return []


def generate_html(data):
    """Generate dashboard HTML with inlined Chart.js graphs.

    Args:
        data: list of metric dicts, each with date, passed, failed, total, rate

    Returns:
        HTML string
    """
    # Inline the data as a JavaScript variable
    data_json = json.dumps(data)

    # Build the table rows for recent reports (limit to 30)
    report_rows = ""
    for entry in reversed(data[-30:]):
        date_str = escape(entry.get("date", ""))
        rate = entry.get("rate", 0.0)
        passed = entry.get("passed", 0)
        total = entry.get("total", 0)
        # Report filename: report-YYYYMMDD.html (remove dashes from date)
        report_file = "report-%s.html" % date_str.replace("-", "")
        report_rows += (
            '<tr><td>%s</td><td>%.1f%%</td><td>%d/%d</td>'
            '<td><a href="%s" target="_blank">View</a></td></tr>'
            % (date_str, rate, passed, total, escape(report_file))
        )

    # Latest metrics display
    latest_html = ""
    if data:
        latest = data[-1]
        latest_html = (
            '<p>Generated: <strong>%s %s</strong></p>'
            '<p style="font-size: 18px; font-weight: bold; color: #2a2;">'
            'Pass Rate: %.1f%% (%d/%d)</p>'
            % (
                escape(latest.get("date", "")),
                escape(latest.get("time", "")),
                latest.get("rate", 0.0),
                latest.get("passed", 0),
                latest.get("total", 0),
            )
        )

    html = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Starfish WPT Status Dashboard</title>
  <style>
    * { box-sizing: border-box; }
    body {
      font: 14px/1.5 system-ui, -apple-system, sans-serif;
      margin: 0;
      padding: 1.5rem;
      color: #1a1a1a;
      background: #f9f9f9;
    }
    h1 {
      font-size: 1.8rem;
      margin: 0 0 0.5rem;
      color: #333;
    }
    h2 {
      font-size: 1.2rem;
      margin: 1.5rem 0 0.75rem;
      color: #333;
      border-bottom: 2px solid #ddd;
      padding-bottom: 0.5rem;
    }
    .section {
      margin: 1.5rem 0;
      padding: 1.5rem;
      background: white;
      border-radius: 6px;
      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
    }
    .meta {
      font-size: 13px;
      color: #666;
      margin: 0.5rem 0 0;
    }
    .chart-container {
      position: relative;
      height: 350px;
      margin-bottom: 1.5rem;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      font-size: 13px;
    }
    th, td {
      padding: 0.5rem 0.75rem;
      border-top: 1px solid #eee;
      text-align: left;
    }
    th {
      background: #f5f5f5;
      font-weight: 600;
      border-top: none;
    }
    td a {
      color: #0066cc;
      text-decoration: none;
    }
    td a:hover {
      text-decoration: underline;
    }
    .no-data {
      padding: 2rem;
      text-align: center;
      color: #999;
    }
  </style>
</head>
<body>
  <h1>Starfish WPT Status Dashboard</h1>
  <div class="meta">Web Platform Tests (WPT) status tracking for Starfish browser engine</div>

  <div class="section">
    <h2>Latest Metrics</h2>
""" + (latest_html if latest_html else '<div class="no-data">No data available yet.</div>') + """
  </div>

  <div class="section">
    <h2>Pass Rate Trend (Last 30 Days)</h2>
    <div class="chart-container">
      <canvas id="rateChart"></canvas>
    </div>
  </div>

  <div class="section">
    <h2>Passed/Failed Count Trend</h2>
    <div class="chart-container">
      <canvas id="countChart"></canvas>
    </div>
  </div>

  <div class="section">
    <h2>Recent Reports</h2>
    <table>
      <thead>
        <tr>
          <th>Date</th>
          <th>Pass Rate</th>
          <th>Passed / Total</th>
          <th>Action</th>
        </tr>
      </thead>
      <tbody>
"""

    if report_rows:
        html += report_rows
    else:
        html += '<tr><td colspan="4"><div class="no-data">No reports yet.</div></td></tr>'

    html += """
      </tbody>
    </table>
  </div>

  <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js"></script>
  <script>
    const WPT_DATA = """ + data_json + """;

    if (WPT_DATA.length > 0) {
      // Pass rate trend chart
      const rateCtx = document.getElementById('rateChart');
      if (rateCtx) {
        new Chart(rateCtx, {
          type: 'line',
          data: {
            labels: WPT_DATA.map(d => d.date),
            datasets: [{
              label: 'Pass Rate (%)',
              data: WPT_DATA.map(d => d.rate),
              borderColor: '#2a2',
              backgroundColor: 'rgba(42, 170, 42, 0.1)',
              borderWidth: 2,
              pointRadius: 4,
              pointBackgroundColor: '#2a2',
              pointBorderColor: 'white',
              pointBorderWidth: 2,
              tension: 0.3,
              fill: true
            }]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
              legend: { display: true, position: 'top' }
            },
            scales: {
              y: {
                beginAtZero: true,
                min: 0,
                max: 100,
                ticks: { callback: v => v + '%' }
              }
            }
          }
        });
      }

      // Passed/Failed count trend chart
      const countCtx = document.getElementById('countChart');
      if (countCtx) {
        new Chart(countCtx, {
          type: 'bar',
          data: {
            labels: WPT_DATA.map(d => d.date),
            datasets: [
              {
                label: 'Passed',
                data: WPT_DATA.map(d => d.passed),
                backgroundColor: '#2a2'
              },
              {
                label: 'Failed',
                data: WPT_DATA.map(d => d.failed),
                backgroundColor: '#e33'
              }
            ]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
              legend: { display: true, position: 'top' }
            },
            scales: {
              x: { stacked: true },
              y: { stacked: true, beginAtZero: true }
            }
          }
        });
      }
    }
  </script>
</body>
</html>"""

    return html


def main(argv):
    p = ArgumentParser(description=__doc__)
    p.add_argument("--data-file", default="data.json",
                   help="cumulative data.json file (default: data.json)")
    p.add_argument("--output", "-o", default="index.html",
                   help="output HTML file (default: index.html)")
    args = p.parse_args(argv)

    data = load_data(args.data_file)
    html = generate_html(data)

    with open(args.output, "w") as fp:
        fp.write(html)

    print("Generated %s with %d data points" % (args.output, len(data)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

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

    python3 tool/wpt/scripts/wpt_generate_dashboard.py \\
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
        # Report filename: report-YYYYMMDD.html (remove dashes from date)
        report_file = "report-%s.html" % date_str.replace("-", "")
        report_rows += (
            '<tr><td>{date}</td><td>{rev}</td><td>{fp:,}/{ft:,}</td>'
            '<td>{rate:.1f}%</td><td>{passed:,}/{total:,}</td>'
            '<td><a href="{file}" target="_blank">View</a></td></tr>'
        ).format(
            date=date_str,
            rev=escape(entry.get("wpt_revision", "—")),
            fp=entry.get("files_passed", 0),
            ft=entry.get("files_total", 0),
            rate=entry.get("rate", 0.0),
            passed=entry.get("passed", 0),
            total=entry.get("total", 0),
            file=escape(report_file),
        )

    # Latest metrics display. wpt.fyi-style "N tests (M subtests)" makes the
    # file count and subtest count both explicit, and the WPT revision pins
    # which checkout produced them (so a shift after a submodule bump is not
    # read as a regression).
    latest_html = ""
    if data:
        latest = data[-1]
        latest_html = (
            '<p>Generated: <strong>{date} {time}</strong></p>'
            '<p>WPT revision: <strong>{rev}</strong></p>'
            '<p style="font-size: 18px; font-weight: bold; color: #2a2;">'
            'Showing {files:,} tests ({total:,} subtests) &middot; '
            'Subtest pass rate {rate:.1f}% ({passed:,}/{total:,} passing)</p>'
        ).format(
            date=escape(latest.get("date", "")),
            time=escape(latest.get("time", "")),
            rev=escape(latest.get("wpt_revision", "—")),
            files=latest.get("files_total", 0),
            total=latest.get("total", 0),
            rate=latest.get("rate", 0.0),
            passed=latest.get("passed", 0),
        )
        # "types" (added alongside reftest/crashtest support) breaks the
        # above testharness-only totals down per test kind. Older records
        # predate this key, so only render it when present -- no schema
        # migration needed for existing history.
        types = latest.get("types")
        if types:
            rows = "".join(
                '<tr><td>{t}</td><td>{fp:,}/{ft:,}</td><td>{rate:.1f}%</td>'
                '<td>{passed:,}/{total:,}</td></tr>'.format(
                    t=escape(kind),
                    fp=m.get("files_passed", 0), ft=m.get("files_total", 0),
                    rate=m.get("rate", 0.0),
                    passed=m.get("passed", 0), total=m.get("total", 0))
                for kind, m in sorted(types.items())
            )
            latest_html += (
                '<table style="margin-top:0.5rem"><thead><tr>'
                '<th>Type</th><th>Files passed/total</th><th>Rate</th>'
                '<th>Subtests passed/total</th></tr></thead>'
                '<tbody>{rows}</tbody></table>'
            ).format(rows=rows)

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
    .note {
      margin: 0.75rem 0 0;
      padding: 0.75rem 1rem;
      background: #fff8e1;
      border: 1px solid #ffe082;
      border-radius: 6px;
      font-size: 13px;
      color: #5d4037;
    }
  </style>
</head>
<body>
  <h1>Starfish WPT Status Dashboard</h1>
  <div class="meta">Web Platform Tests (WPT) status for Starfish &mdash; counted at the subtest level, comparable to wpt.fyi.</div>
  <div class="note"><strong>Scope:</strong> the headline totals, trend charts, and Recent Reports table combine every test type the nightly run covers (testharness + reftest + crashtest when all three run; wdspec still excluded), so the total test count is smaller than wpt.fyi's testharness-only set -- compare at the subtest level, not by raw totals. testharness is counted per subtest; reftest/crashtest are whole-file pass/fail, so the combined number blends two units. The per-type split (with the same fields) is in the table below the headline; older reports predate reftest/crashtest and are testharness only throughout.</div>

  <div class="section">
    <h2>Latest Metrics</h2>
""" + (latest_html if latest_html else '<div class="no-data">No data available yet.</div>') + """
  </div>

  <div class="section">
    <h2>Failing Subtest Count Trend (Last 30 Days)</h2>
    <div class="chart-container">
      <canvas id="failChart"></canvas>
    </div>
  </div>

  <div class="section">
    <h2>Subtest Pass Rate Trend (Last 30 Days)</h2>
    <div class="chart-container">
      <canvas id="rateChart"></canvas>
    </div>
  </div>

  <div class="section">
    <h2>Passed/Failed Subtest Count Trend</h2>
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
          <th>WPT revision</th>
          <th>Tests passed / total</th>
          <th>Subtest Pass Rate</th>
          <th>Subtests passed / total</th>
          <th>Action</th>
        </tr>
      </thead>
      <tbody>
"""

    if report_rows:
        html += report_rows
    else:
        html += '<tr><td colspan="6"><div class="no-data">No reports yet.</div></td></tr>'

    html += """
      </tbody>
    </table>
  </div>

  <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js"></script>
  <script>
    const WPT_DATA = """ + data_json + """;

    if (WPT_DATA.length > 0) {
      // Failing subtest count trend chart (mirrors wpt.fyi's top "Browser
      // Specific Failures" graph; Starfish has no cross-browser data, so this
      // tracks our own failing-subtest count over time).
      const failCtx = document.getElementById('failChart');
      if (failCtx) {
        new Chart(failCtx, {
          type: 'line',
          data: {
            labels: WPT_DATA.map(d => d.date),
            datasets: [{
              label: 'Failing Subtests',
              data: WPT_DATA.map(d => d.failed),
              borderColor: '#e33',
              backgroundColor: 'rgba(227, 51, 51, 0.1)',
              borderWidth: 2,
              pointRadius: 4,
              pointBackgroundColor: '#e33',
              pointBorderColor: 'white',
              pointBorderWidth: 2,
              tension: 0.3,
              fill: true
            }]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: { legend: { display: true, position: 'top' } },
            scales: { y: { beginAtZero: true } }
          }
        });
      }

      // Pass rate trend chart
      const rateCtx = document.getElementById('rateChart');
      if (rateCtx) {
        new Chart(rateCtx, {
          type: 'line',
          data: {
            labels: WPT_DATA.map(d => d.date),
            datasets: [{
              label: 'Subtest Pass Rate (%)',
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

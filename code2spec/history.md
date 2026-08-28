# Code2Spec Execution History

> `코드 규모`는 AST 파싱된 소스 파일 기준입니다. (hidden dot 경로 제외, 실행 시점 min_lines 설정 반영)
> History format: v2 hybrid

## Legacy Notes

# Code2Spec History
## W1: code2spec-discovery (2026-08-27)
### Execution Summary
- **Tool version:** v0.5.3
- **Analysis target:** `` (full project)
- **Document language:** English
- **AX Artifact restore:** skipped (MCP not available)
### Step Completion
| Step | Description | Status | Duration |
| 1 | Prerequisites check | Passed | — |
| 2 | Output path & language | Confirmed | — |
| 3-1 | AST export (initial) | 330 files, 7004 nodes, 31096 edges | — |
| 3-1b | IPC pattern discovery | 0 custom patterns (C++ codebase) | — |
| 3-1d | Final AST export | 330 cached, 0 changed | — |
| 3-1e | LLM extraction (ENUM/Constant/IPC) | 63 enums, 446 constants, 73 IPC patterns | — |
| 3-1b | Interface scan | 34 candidates | — |
| 3-2 | Module discovery | 15 modules identified (HITL approved) | — |
| 3-3 | Core candidate analysis | 15 Core, 0 Peripheral | — |
| 3-6 | Config save | analysis_scope=3 (Detail) | — |
| 3-6b | Diagram generation | architecture.mmd + dependency-heatmap.mmd | — |
| 3-7 | Runtime initialization | Session 20260827_112419 | — |
| 3-8 | Core manifest | 330 core files | — |
| 3-9 | W2 progress init | 15 modules initialized | — |
| 5 | System document generation | 9 SDD chapters + FR index + README | — |
### W1 Runtime
- **Duration:** 46 seconds (workflow tracking)
- **Total duration:** ~30 minutes (including LLM processing)
- **Terminal status:** Passed
### Generated Artifacts
**SDD Chapters:**
- `01-introduction.md`
- `02-architecture.md`
- `03-design-patterns.md`
- `04-data-layer.md`
- `05-external-interfaces.md`
- `06-configuration-deployment.md`
- `07-resources.md`
- `08-security-quality.md`
- `09-ipc-enum-catalog.md`
**Other:**
- `README.md`
- `history.md`
- `functional-requirements/index.md`
- `diagrams/architecture.mmd`
- `diagrams/dependency-heatmap.mmd`
### Module List (15 modules)
1. engine-core (11 files)
2. js-binding (38 files)
3. browser-history (2 files)
4. worker-launcher (2 files)
5. platform-canvas (25 files)
6. platform-loader (17 files)
7. platform-network (17 files)
8. platform-multimedia (33 files)
9. platform-message-loop (12 files)
10. platform-system (18 files)
11. embedding-api (57 files)
12. shell (39 files)
13. test-tooling (51 files)
14. third-party-libs (5 files)
15. docs-webapi (3 files)
### Next Steps
- **W2 (code2spec-modules):** Deep module analysis + Module Design Cards + individual FR documents
- **W3 (code2spec-finalize):** Interlinking + Quick Reference + History finalization

## Summary

| 실행 일시 | Spec 경로 | 유형 | 모드 | 버전 | 모듈 | 코드 규모 | 총 소요 | MD 산출물 | 비고 |
|---|---|---|---|---|---:|---|---:|---|---|
| 2026-08-27 11:24:19 | code2spec/ | full | Detail | 0.5.3 | 15 | 330파일 / 102,438행 / 5,630함수 / 373클래스 | 16분 3초 | 45개 / 2,327행 | W1+W2+W3 terminal status recorded |

## 품질 메트릭

| 실행 일시 | Spec 경로 | 유형 | 모드 | Evidence Precision | Source Coverage | Core Coverage | Broken Source Link Rate | Quality Gate |
|---|---|---|---|---:|---:|---:|---:|---|
| 2026-08-27 11:24:19 | code2spec/ | full | Detail | 0.00% (0/45) | 100.00% (330/330) | 100.00% (15/15 모듈) | 100.00% (45/45) | FAIL (2 issues) |

## Workflow Details

<details>
<summary>2026-08-27 11:24:19 — full / Detail</summary>

- Coding assistants: cline

| Workflow | Duration |
|---|---:|
| W1 | 46초 |
| W2 | 8분 37초 |
| W3 | 6분 40초 |

</details>

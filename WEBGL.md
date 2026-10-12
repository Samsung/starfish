# WebGL Development and Regression Prevention Guide

> **[경고 / 절대적 철칙 (Non-Negotiable Mandates)]**
> 1. **커밋 및 푸시 전 크로노스 WebGL 테스트 전수 로컬 실행 필수**:
>    - WebGL 코드를 한 줄이라도 수정하거나 신규 기능을 추가/활성화했을 때, **개별 테스트 몇 개만 확인하고 커밋/푸시하는 행위는 엄격히 금지**됩니다.
>    - 반드시 로컬에서 활성화된 전체 크로노스 WebGL 테스트 수트(`vendor_test_khronos2` 또는 `khronos_test`)를 **끝까지 전수 실행하여 `Fail: 0` 및 회귀(Regression) 0건을 직접 검증한 후에만 커밋 및 푸시**해야 합니다.
> 2. **디버그/테스트 모드 어설션 검증 필수**:
>    - CI 환경은 테스트/디버그 모드(`-DENABLE_TEST=1`)로 빌드되어 내부 `STARFISH_ASSERT`가 활성화됩니다.
>    - 릴리즈 빌드에서 에러가 안 나더라도 디버그 모드에서 어설션 크래시가 날 수 있으므로 반드시 디버그/테스트 모드에서 검증해야 합니다.
> 3. **`ENTER_CONTEXT_SCOPE()` 중첩 진입 절대 금지**:
>    - `GLContextScope`는 재진입(중첩)을 허용하지 않으며, 중첩 진입 시 `Assertion currentContext.isValid() == false` 크래시가 발생합니다.
>    - 자식 클래스(`WebGL2RenderingContext`)에서 부모 클래스(`WebGLRenderingContext`)의 메서드를 호출하여 위임할 때는 자식 클래스에서 절대 `ENTER_CONTEXT_SCOPE();`를 열지 마십시오.
> 4. **`docs/Spec.md` 동기화 필수**:
>    - WebGL 기능 구현 및 테스트 활성화 시 반드시 `docs/Spec.md`의 `WebGL 2.0 Implementation Status` 표와 목록을 같은 커밋에서 항상 함께 갱신해야 합니다 (`review-bot` 필수 규칙).
> 5. **Tidy 검사 필수**:
>    - `python3 ./tool/lint/check_tidy.py` 실행 시 `Total number of errors: 0`이어야 합니다.

---

## 1. 빌드 설정

WebGL 기능은 기본적으로 비활성화되어 있으므로 `-DWEBGL=1` 옵션과 Cairo GL 백엔드가 필요합니다. 테스트 및 어설션 검증을 위해 `-DENABLE_TEST=1`을 반드시 활성화합니다.

```bash
# WebGL 테스트 빌드 설정
cmake CMakeLists.txt -H./ -Bbuild/out_linux_release \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBACKEND=glib_cairo_gl \
  -DTARGETNAME=Starfish \
  -G Ninja \
  -DSHELL=x11 \
  -DWEBGL=1 \
  -DUSE_FFMPEG_MEDIA_PLAYER=1 \
  -DENABLE_TEST=1

# 빌드 실행
ninja -C build/out_linux_release Starfish
```

> **주의:** IDL(`.idl`) 파일을 수정한 경우, CMake configure를 다시 실행해야 C++ 바인딩 코드가 재생성됩니다.

---

## 2. 코드 스타일 & Tidy 검사 (필수)

커밋 전 반드시 Tidy 및 Clang-Format 검사를 수행하여 코드 스타일 에러를 방지합니다.

### 2.1 check_tidy.py 사용 (공식 도구)
```bash
# WebGL 관련 디렉토리 Tidy 검사
python3 ./tool/lint/check_tidy.py src/core/dom/canvas/webgl src/platform/canvas/gl

# Tidy 스타일로 자동 포맷팅 적용
python3 ./tool/lint/check_tidy.py --update src/core/dom/canvas/webgl src/platform/canvas/gl
```

### 2.2 git-clang-format 사용
Staged 변경사항에 대해 즉시 포맷 검증:
```bash
git add <수정된_파일들>
git-clang-format HEAD
```
`clang-format did not modify any files`가 출력되어야 합니다.

---

## 3. 크로노스 WebGL 테스트 로컬 전수 검증 절차 (필수)

WebGL 작업 시 사소한 변경으로도 기존 수백 개의 WebGL 1.0 / 2.0 테스트들에 수많은 리그레션(어설션 크래시, FBO 바인딩 오류 등)이 발생할 수 있습니다.
**반드시 로컬에서 크로노스 WebGL 테스트를 전수 실행하여 회귀가 없음을 확인한 후에 커밋/푸시해야 합니다.**

### 3.1 로컬 테스트 HTTP 서버 실행
Khronos 테스트는 하네스 브릿지(`inject_report.js`) 주입이 필요하므로 아래 명령으로 로컬 서버를 백그라운드 구동합니다.

```bash
python3 tool/runner/http_server.py -d third_party/webgl --bind localhost 11010 \
  --inject-script tool/khronos/inject_report.js \
  --inject-into /js-test-pre.js --inject-into /more/unit.js &
```

### 3.2 개별 테스트 단독 디버깅 실행
수정 중인 단일 HTML 테스트 파일의 통과 여부 및 콘솔 에러를 확인할 때:

```bash
env no_proxy=localhost,127.0.0.1 NO_PROXY=localhost,127.0.0.1 http_proxy= https_proxy= \
  xvfb-run -a ./Starfish \
  "http://localhost:11010/conformance-suites/2.0.0/conformance2/query/query.html" \
  --hide-window --width=800 --height=600 --regression-test
```
출력에서 `KHRONOS DONE pass=<n> fail=0 skipped=<n>` 및 `[PASS]`를 확인합니다.

### 3.3 크로노스 전체 테스트 수트 전수 검증 (커밋/푸시 전 필수)
Starfish 공식 테스트 러너(`test_runner.py`)를 통해 활성화된 전체 테스트 목록을 전수 검증합니다.

```bash
# 1. WebGL 2.0 활성 테스트 전체 전수 실행 (tool/reftest/cairo/khronos_webgl2.res)
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py vendor_test_khronos2

# 2. WebGL 1.0 활성 테스트 전체 전수 실행 (tool/reftest/cairo/khronos_webgl.res)
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py vendor_test_khronos

# 3. 전체 Khronos 테스트 스위트 일괄 실행
xvfb-run -s '-screen 0 1920x1080x24' -a ./tool/runner/test_runner.py khronos_test
```
> **최종 출력 확인**: 러너 종료 시 출력되는 통계표에서 **Fail: 0**을 반드시 확인해야 합니다.

---

## 4. 테스트 리스트(`*.res`) 및 Spec.md 동기화 규칙

1. **100% ALL PASS 원칙**:
   - `tool/reftest/cairo/khronos_webgl2.res` (또는 `khronos_webgl.res`)에서 주석(`#`)이 해제된 테스트는 CI에서 회귀 검사 대상으로 동작합니다.
   - 단 하나의 assertion이라도 실패(`fail > 0`)하거나 타임아웃이 나는 테스트는 절대 주석을 풀어서는 안 됩니다.

2. **docs/Spec.md 동기화 (항상 필수)**:
   - 기능 구현 또는 테스트 활성화 시 **반드시 `docs/Spec.md`의 WebGL 구현 현황을 항상 같이 업데이트**해야 합니다.
   - `Object handles`: 신규 지원된 인터페이스 객체(`WebGLQuery`, `WebGLSampler`, `WebGLTransformFeedback` 등)를 구현 완료로 이동하고 `mostly unimplemented` 등의 문구를 제거.
   - `Implemented core features`: 새로 구현된 기능과 API 메서드 목록을 상세히 추가.
   - `Unimplemented WebGL 2.0 features`: 구현 완료된 항목을 제거.
   - **코드 변경 커밋 시 `docs/Spec.md` 변경사항을 같은 커밋에 반드시 포함**해야 합니다.

---

## 5. C++ 구현 주의사항

### 5.1 GLContextScope 중첩 진입 금지
`ENTER_CONTEXT_SCOPE()`는 스레드당 단 1개의 컨텍스트 스코프만 허용합니다.
자식 클래스 오버라이드 함수에서 부모 클래스 함수를 호출할 때:
```cpp
// [잘못된 예시 - 중첩 스코프로 어설션 크래시 발생]
void WebGL2RenderingContext::clear(uint32_t mask)
{
    ENTER_CONTEXT_SCOPE(); // 진입
    ...
    WebGLRenderingContext::clear(mask); // 내부에서 또 ENTER_CONTEXT_SCOPE() 진입 -> CRASH!
}

// [올바른 예시]
void WebGL2RenderingContext::clear(uint32_t mask)
{
    // 추가 유효성 검사 수행
    if (mask & GL_COLOR_BUFFER_BIT) {
        ...
    }
    // 부모 클래스가 자체적으로 컨텍스트 스코프를 관리하므로 위임 호출만 수행
    WebGLRenderingContext::clear(mask);
}
```

### 5.2 플랫폼 호환성 (EvasGL / Tizen)
`src/platform/canvas/gl/EvasGL.cpp`에 GLES 3.0 신규 API를 추가할 때는 Tizen EFL 헤더 버전에 따라 함수 포인터 필드가 없을 수 있으므로 반드시 매크로 가드와 런타임 폴백을 적용해야 합니다:
```cpp
void someGLES3Function(...) override
{
#if defined(EVAS_GL_GLES3)
    if (m_evasGLAPI->glSomeGLES3Function) {
        m_evasGLAPI->glSomeGLES3Function(...);
        return;
    }
#endif
    STARFISH_UNSUPPORTED("glSomeGLES3Function");
}
```

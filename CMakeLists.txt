# CMakeList.txt: CNC_quote_program에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
cmake_minimum_required (VERSION 3.10)

# 지원되는 경우 MSVC 컴파일러에 대해 핫 다시 로드 사용하도록 설정합니다.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CNC_quote_program")

# ⚠️ Qt UI 컴파일에 필요한 필수 자동화 옵션 활성화 (구조 유지용 필수 추가)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 패키지 탐색 (vcpkg 툴체인이 적용된 상태에서 정상 추적됩니다)
if (WIN32)
  list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
endif()
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(OpenCASCADE REQUIRED)
find_package(pybind11 REQUIRED)

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (CNC_quote_program "CNC_quote_program.cpp" "CNC_quote_program.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CNC_quote_program PROPERTY CXX_STANDARD 20)
endif()

# ⚠️ 탐색된 라이브러리들을 프로젝트 대상(Target)에 의존성 링크해 줍니다.
target_link_libraries(CNC_quote_program PRIVATE 
    Qt6::Widgets
    TKOpenGl        # Open CASCADE 3D 그래픽 구동용 핵심 라이브러리
    TKDESTEP        # Open CASCADE STEP 파일 해석용 핵심 라이브러리 (OCCT 8.0+)
    pybind11::embed # Python AI 엔진 임베딩 교량용 라이브러리
)

# Windows 환경에서 실행 시 Qt6 DLL들을 빌드 결과물 폴더에 자동 복사해 줍니다.
if (WIN32)
  add_custom_command(TARGET CNC_quote_program POST_BUILD
    COMMAND "C:/Qt/6.11.1/msvc2022_64/bin/windeployqt.exe"
            $<$<CONFIG:Debug>:--debug>
            $<$<NOT:$<CONFIG:Debug>>:--release>
            $<TARGET_FILE:CNC_quote_program>
    COMMENT "Copying Qt runtime DLLs..."
  )
endif()

# TODO: 필요한 경우 테스트를 추가하고 대상을 설치합니다.
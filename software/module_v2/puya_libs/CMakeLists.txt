project(PuyaBSP  LANGUAGES  C ASM)

add_library(${PROJECT_NAME} OBJECT 
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/PY32F0xx/Source/system_py32f0xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_irda.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_rcc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_lptim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_comp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_adc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_tim_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_wwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_iwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_rtc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Src/py32f0xx_hal_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/Src/py32f0xx_bsp_printf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/Src/py32f0xx_bsp_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/Src/py32f0xx_bsp_clock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/PY32F0xx/Source/gcc/startup_py32f003.s
)

target_include_directories(${PROJECT_NAME} PUBLIC
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/PY32F0xx_HAL_Driver/Inc
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP/Inc
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/PY32F0xx/Include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Core/Include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Core/Include
    )

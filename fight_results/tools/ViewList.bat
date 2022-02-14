@echo off
setlocal EnableDelayedExpansion

:while1

set list=Zen6VScgi_C_async_4c2g_s_1 Zen6VScgi_C_async_4c2g_s_2 Zen6VScgi_C_async_4c2g_sd_1 Zen6VScgi_C_async_4c2g_sd_2 Zen6VScgi_C_async_4c2g_sbtd_1 Zen6VScgi_C_async_4c2g_sbtd_2

for %%a in (%list%) do (
	echo %%a
	ViewResults.exe %%a
)

timeout /t 60

goto :while1
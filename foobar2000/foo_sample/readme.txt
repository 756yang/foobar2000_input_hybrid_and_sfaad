This component demonstrates:
* main.cpp :
  * 声明你的组件的版本信息
  * Declaring your component's version information.
* input_raw.cpp :
  * 声明你为解码额外音频格式的输入类
  * Declaring your own "input" classes for decoding additional audio file formats.
  * 调用文件系统服务
  * Calling file system services.
* preferences.cpp :
  * 声明你的配置变量
  * Declaring your configuration variables.
  * 使用简单的WTL对话框创建首选项页面
  * Creating preferences pages using simple WTL dialogs.
  * 声明高级首选项入口
  * Declaring advanced preferences entries.
* initquit.cpp :
  * 简单的初始化/关闭回调服务
  * Sample initialization/shutdown callback service.
* dsp.cpp :
  * 简单的DSP（数字信号处理器）
  * Sample DSP.
* contextmenu.cpp :
  * 简单的上下文菜单命令
  * Sample context menu command.
* decode.cpp :
  * 从任意音频文件获取PCM数据
  * Getting PCM data from arbitrary audio files.
  * 在带进度对话框的工作线程中使用threaded_process API以易于运行耗时间的任务
  * Use of the threaded_process API to easily run time-consuming tasks in worker threads with progress dialogs.
* mainmenu.cpp :
  * 简单的主菜单命令
  * Sample main menu command
* playback_state.cpp :
  * 使用回放回调
  * Use of playback callbacks.
  * 使用回访控制
  * Use of playback control.
* ui_element.cpp : 
  * 简单的UI元件接口
  * Simple UI Element implementation.
* rating.cpp
  * 使用metadb_index_client的最小rating+comment接口
  * Minimal rating+comment implementation using metadb_index_client.
  * 使用metadb_display_field_provider通过标题格式显示你的数据
  * Present your data via title formatting using metadb_display_field_provider.
  * 使用track_property_provider在属性对话框中显示你的数据
  * Present your data in the properties dialog using track_property_provider.
  * 实用菜单项
  * Utility menu items.
  * 流格式化程序的基本用法
  * Basic use of stream formatters.
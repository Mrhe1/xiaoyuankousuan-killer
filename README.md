xaioyuansouti-killer
基于C++/OpenCV的小猿口算自动图像识别+答题
*************只有屏幕分辨率为2560*1440可以直接使用此代码（否则需要改截屏的像素位置）
一.依赖的库和模拟器
    1.opencv（推荐4.1.0）
    2.abd
    3.模拟器mumu模拟器
二.程序使用方法
    1.安装必要库和模拟器
    2.开启模拟器adb调试，并用命令行测试adb connect是否成功
    3.用vs打开解决方案编译程序（或者试一下x64\Debug中的程序可否直接运行）
    4.打开程序和模拟器设置模拟器分辨率为2k（默认横屏模式）
    5.待程序显示press enter to start按下enter键
    6.在模拟器中进入小猿口算的口算pk，选择20以内或者100以内比大小开始pk
三.程序流程
    1.通过BitBlt在win系统中截图（耗时13ms）
    2.用opencv把处理截图并与模板图片比较相似度（20ms）
    3.把命令用套数字发送给adb服务器（相比调用shell延迟更低，稳定性更好）
四.可改动的地方
    1.截图和图像识别可以多线程甚至用cuda加速，可以自己搞
    2.主程序while最后的延迟可以调（预设的延迟可以做到500ms一道题，延迟太低会导致上一道题的答案写到下一道题去）
    3.adb划线的时间也可以改（也就是主程序draw_polyline中的duration参数）

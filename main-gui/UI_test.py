# test_color_dialog.py
import sys
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtGui import QColor

from qfluentwidgets import ColorDialog


def handle_color_change(color):
    print("Selected color:", color.name())


if __name__ == '__main__':
    app = QApplication(sys.argv)

    # 创建一个主窗口作为父组件
    main_window = QMainWindow()
    main_window.setFixedSize(800, 1200)
    main_window.show()

    # 初始颜色设为红色
    initial_color = QColor(255, 0, 0)

    # 创建颜色对话框，传入有效的父窗口
    dialog = ColorDialog(
        color=initial_color,
        title="选择颜色",
        parent=main_window,  # 这里改为有效的父窗口
        enableAlpha=True
    )

    # 连接颜色变化信号
    dialog.colorChanged.connect(handle_color_change)

    # 显示对话框
    dialog.show()

    sys.exit(app.exec_())

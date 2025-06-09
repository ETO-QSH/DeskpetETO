# -*- coding: utf-8 -*-
from PyQt5.QtCore import Qt, pyqtSignal, QMimeData, QTimer, QPoint
from PyQt5.QtGui import QDrag, QPixmap, QImage
from PyQt5.QtWidgets import QApplication, QLabel, QWidget, QVBoxLayout
from qfluentwidgets import ElevatedCardWidget
import win32gui
import win32con
import win32api
from PIL import ImageGrab
import sys


class WindowCard(ElevatedCardWidget):
    windowRestored = pyqtSignal(int)  # 参数为HWND

    def __init__(self, parent=None):
        super().__init__(parent)
        self.hwnd = None
        self._setup_ui()
        self.setAcceptDrops(True)
        self.setFixedSize(150, 150)

    def _setup_ui(self):
        self.image_label = QLabel(self)
        self.image_label.setAlignment(Qt.AlignCenter)
        self.image_label.setGeometry(5, 5, 140, 140)

    def is_valid_window(self, hwnd):
        """验证窗口有效性"""
        return hwnd and \
            win32gui.IsWindow(hwnd) and \
            win32gui.IsWindowVisible(hwnd) and \
            win32gui.GetWindowText(hwnd) != ''

    def capture_thumbnail(self, hwnd):
        """安全获取窗口缩略图"""
        try:
            if not self.is_valid_window(hwnd):
                return QPixmap()

            # 获取窗口区域
            left, top, right, bottom = win32gui.GetWindowRect(hwnd)
            if right <= left or bottom <= top:
                return QPixmap()

            # 使用PIL捕获
            img = ImageGrab.grab(bbox=(left, top, right, bottom))
            img = img.convert("RGB")
            img = img.resize((140, 140))

            # 转换为QPixmap
            qimage = QImage(img.tobytes(), img.width, img.height, QImage.Format_RGB888)
            return QPixmap.fromImage(qimage)
        except Exception as e:
            print(f"截图失败: {e}")
            return QPixmap()

    def store_window(self, hwnd):
        """存储窗口"""
        try:
            if self.hwnd == hwnd or not self.is_valid_window(hwnd):
                return

            # 如果已有窗口则先恢复
            if self.hwnd:
                self.restore_window(force=True)

            # 隐藏目标窗口
            win32gui.ShowWindow(hwnd, win32con.SW_HIDE)
            self.hwnd = hwnd

            # 更新缩略图
            pixmap = self.capture_thumbnail(hwnd)
            if not pixmap.isNull():
                self.image_label.setPixmap(pixmap)
        except Exception as e:
            print(f"收纳失败: {e}")

    def restore_window(self, force=False):
        """恢复窗口（双击触发）"""
        try:
            if not self.hwnd:
                return

            if self.is_valid_window(self.hwnd):
                # 恢复窗口
                win32gui.ShowWindow(self.hwnd, win32con.SW_SHOW)
                win32gui.BringWindowToTop(self.hwnd)
                win32gui.SetForegroundWindow(self.hwnd)
                self.windowRestored.emit(self.hwnd)

            # 强制清除或自然清除
            if force or not self.is_valid_window(self.hwnd):
                self.hwnd = None
                self.image_label.clear()
        except Exception as e:
            print(f"恢复失败: {e}")
            self.hwnd = None
            self.image_label.clear()

    # 拖放事件处理
    def dragEnterEvent(self, event):
        if event.mimeData().hasFormat("application/hwnd"):
            event.acceptProposedAction()

    def dropEvent(self, event):
        if event.mimeData().hasFormat("application/hwnd"):
            try:
                hwnd = int(event.mimeData().data("application/hwnd").data().decode())
                self.store_window(hwnd)
            except Exception as e:
                print(f"无效窗口句柄: {e}")
            event.acceptProposedAction()

    def mouseDoubleClickEvent(self, event):
        self.restore_window()
        event.accept()


class GlobalDragDetector(QWidget):
    dragStart = pyqtSignal(int)  # 发送HWND

    def __init__(self):
        super().__init__()
        self.dragging = False
        self.drag_hwnd = None
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.check_drag)
        self.timer.start(50)  # 每50ms检测一次

    def get_window_under_cursor(self):
        """获取光标下的有效窗口"""
        try:
            pos = win32api.GetCursorPos()
            hwnd = win32gui.WindowFromPoint(pos)

            # 循环查找顶层父窗口
            parent = win32gui.GetParent(hwnd)
            while parent:
                hwnd = parent
                parent = win32gui.GetParent(hwnd)

            if self.is_valid_drag_target(hwnd):
                return hwnd
        except Exception as e:
            print(f"窗口检测错误: {e}")
        return None

    def is_valid_drag_target(self, hwnd):
        """验证是否为有效可拖动窗口"""
        if not hwnd or hwnd == self.winId().__int__():
            return False

        return win32gui.IsWindowVisible(hwnd) and \
            win32gui.GetWindowText(hwnd) != '' and \
            not self.is_system_window(hwnd)

    def is_system_window(self, hwnd):
        """过滤系统窗口"""
        className = win32gui.GetClassName(hwnd)
        return className in ['Progman', 'WorkerW', 'Shell_TrayWnd', 'Button']

    def check_drag(self):
        """检测拖拽操作"""
        if QApplication.mouseButtons() & Qt.LeftButton:
            if not self.dragging:
                # 开始拖动
                self.drag_hwnd = self.get_window_under_cursor()
                if self.drag_hwnd:
                    self.dragging = True
                    self.start_pos = win32api.GetCursorPos()
            else:
                # 检测拖动距离
                current_pos = win32api.GetCursorPos()
                dx = abs(current_pos[0] - self.start_pos[0])
                dy = abs(current_pos[1] - self.start_pos[1])

                if dx > 10 or dy > 10:
                    self.handle_drag()
                    self.dragging = False
        else:
            self.dragging = False

    def handle_drag(self):
        """处理拖拽操作"""
        if self.drag_hwnd and self.is_valid_drag_target(self.drag_hwnd):
            # 创建虚拟拖动对象
            drag = QDrag(self)
            mime = QMimeData()
            mime.setData("application/hwnd", str(self.drag_hwnd).encode())

            # 生成缩略图
            try:
                img = ImageGrab.grab(win32gui.GetWindowRect(self.drag_hwnd))
                img = img.resize((100, 100))
                qimage = QImage(img.tobytes(), img.width, img.height, QImage.Format_RGB888)
                drag.setPixmap(QPixmap.fromImage(qimage))
            except:
                drag.setPixmap(QPixmap(100, 100))

            drag.setMimeData(mime)
            drag.exec_(Qt.MoveAction)


class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("窗口收纳管理器")
        self.setGeometry(300, 300, 600, 400)

        layout = QVBoxLayout()
        self.card1 = WindowCard()
        self.card2 = WindowCard()
        layout.addWidget(self.card1)
        layout.addWidget(self.card2)

        # 状态显示
        self.status = QLabel("拖动任意窗口到卡片上进行收纳，双击卡片恢复窗口")
        layout.addWidget(self.status)

        self.setLayout(layout)

        # 全局拖放检测
        self.detector = GlobalDragDetector()

        # 连接信号
        self.card1.windowRestored.connect(lambda h: self.update_status(f"已恢复窗口 {h}"))
        self.card2.windowRestored.connect(lambda h: self.update_status(f"已恢复窗口 {h}"))

    def update_status(self, message):
        self.status.setText(message)
        QTimer.singleShot(3000, lambda: self.status.setText("拖动任意窗口到卡片上进行收纳，双击卡片恢复窗口"))


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())

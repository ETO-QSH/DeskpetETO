import sys
from pathlib import Path

from PyQt5.QtCore import Qt, QSize, pyqtSignal, QRectF
from PyQt5.QtGui import QPixmap, QPainter, QPainterPath, QColor, QPen
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLineEdit, QLabel, QPushButton, QFileDialog
from qfluentwidgets import TeachingTip, TeachingTipTailPosition, InfoBarIcon, isDarkTheme


class ImageDropWidget(QWidget):
    pathChanged = pyqtSignal(str)

    def __init__(self, default_image=None, parent=None):
        super().__init__(parent)
        self.setAcceptDrops(True)
        self.default_image = default_image
        self.current_path = ""
        self.target_line_edit = None

        self._hover = False
        self._border_color = QColor("#A0A0A0")

        # 初始化UI
        self.setFixedSize(300, 300)
        self.setObjectName("imageDropWidget")

        self.image_label = QLabel(self)
        self.image_label.setAlignment(Qt.AlignCenter)
        self.image_label.setGeometry(10, 10, 280, 280)

        # 加载默认图片
        self._load_default_image()

        # 启用悬停跟踪
        self.setAttribute(Qt.WA_Hover)
        self.setMouseTracking(True)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHints(QPainter.Antialiasing | QPainter.SmoothPixmapTransform)

        # 绘制背景
        bg_color = QColor(32, 32, 32) if isDarkTheme() else QColor(245, 245, 245)
        path = QPainterPath()
        rect = QRectF(self.rect()).adjusted(2, 2, -2, -2)  # 内边距
        path.addRoundedRect(rect, 15, 15)  # 圆角半径
        # painter.fillPath(path, bg_color)

        # 绘制边框
        border_color = QColor("#3AA3FF" if self._hover else "#A0A0A0")
        if isDarkTheme():
            border_color = QColor("#3AA3FF" if self._hover else "#707070")

        pen = QPen(border_color, 3)  # 线宽
        pen.setDashPattern([4, 4])  # 4像素实线+4像素间隔
        painter.setPen(pen)
        painter.drawPath(path)

    def enterEvent(self, event):
        self._hover = True
        self.update()

    def leaveEvent(self, event):
        self._hover = False
        self.update()

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            self._hover = True
            event.acceptProposedAction()
        else:
            super().dragEnterEvent(event)

    def dragMoveEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def _load_default_image(self):
        if self.default_image:
            self._set_pixmap(self.default_image)
        else:
            self.image_label.setText("拖放图片到此区域")

    def _set_pixmap(self, pixmap: QPixmap):
        scaled = pixmap.scaled(
            self.image_label.size() - QSize(10, 10),
            Qt.KeepAspectRatio,
            Qt.SmoothTransformation
        )
        self.image_label.setPixmap(scaled)

    def bindLineEdit(self, line_edit: QLineEdit):
        self.target_line_edit = line_edit
        line_edit.textChanged.connect(self._update_image)
        self._update_image(line_edit.text())

    def _update_image(self, path: str):
        self.current_path = path
        if path and Path(path).exists():
            pixmap = QPixmap(path)
            if not pixmap.isNull():
                self._set_pixmap(pixmap)
                return
        self._load_default_image()

    def dropEvent(self, event):
        if event.mimeData().hasUrls():
            for url in event.mimeData().urls():
                path = url.toLocalFile()
                if self._is_valid_image(path):
                    self.target_line_edit.setText(path)
                    self.pathChanged.emit(path)
                    event.accept()
                    return
            self._show_error_tip()
        super().dropEvent(event)

    def _is_valid_image(self, path: str):
        return Path(path).suffix.lower() in ('.png', '.jpg', '.jpeg')

    def _show_error_tip(self):
        TeachingTip.create(
            target=self,
            icon=InfoBarIcon.ERROR,
            title='无效文件',
            content='仅支持 PNG/JPG 格式的图片文件',
            tailPosition=TeachingTipTailPosition.BOTTOM,
            duration=3000,
            parent=self.window()
        )


class TestWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("图片拖放测试")
        self.resize(400, 500)

        # 创建组件
        self.line_edit = QLineEdit()
        self.drop_widget = ImageDropWidget(QPixmap("./resource/404.png"))
        self.drop_widget.bindLineEdit(self.line_edit)

        # 创建控制按钮
        self.btn_clear = QPushButton("清除路径")
        self.btn_browse = QPushButton("浏览图片")

        # 布局
        layout = QVBoxLayout()
        layout.addWidget(QLabel("图片路径:"))
        layout.addWidget(self.line_edit)
        layout.addWidget(self.btn_browse)
        layout.addWidget(self.btn_clear)
        layout.addSpacing(20)
        layout.addWidget(self.drop_widget, 0, Qt.AlignCenter)
        layout.addStretch()
        self.setLayout(layout)

        # 连接信号
        self.btn_clear.clicked.connect(self._clear_path)
        self.btn_browse.clicked.connect(self._browse_image)

    def _clear_path(self):
        self.line_edit.clear()

    def _browse_image(self):
        path, _ = QFileDialog.getOpenFileName(self, "选择图片", "./", "图片文件 (*.png *.jpg *.jpeg)")
        if path:
            self.line_edit.setText(path)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = TestWindow()
    window.show()
    sys.exit(app.exec_())

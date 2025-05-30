from PyQt5.QtCore import pyqtSignal, Qt, QSize, QRect, QPoint
from PyQt5.QtGui import QFont, QPixmap
from PyQt5.QtWidgets import QLabel

from qfluentwidgets import ElevatedCardWidget


class ClickableElevatedCardWidget(ElevatedCardWidget):
    """ 支持双状态切换的可点击卡片控件 """
    leftClicked = pyqtSignal()
    rightClicked = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._current_mode = "text"  # text/image
        self._last_image_path = ""
        self._placeholder_text = "左 键  ->  简 单 渲 染 测 试\n右 键  ->  打 开 外 部 工 具"

        # 初始化子控件
        self._init_ui()

        # 默认显示占位文字
        self.show_text(self._placeholder_text)

    def _init_ui(self):
        """ 初始化界面组件 """
        # 文字标签
        self.text_label = QLabel(self)
        self.text_label.setAlignment(Qt.AlignCenter)
        self.text_label.setFont(QFont("萝莉体", 12))
        self.text_label.setWordWrap(True)
        self.text_label.setStyleSheet("""color: gray;""")

        # 图片标签
        self.image_label = QLabel(self)
        self.image_label.setAlignment(Qt.AlignCenter)
        self.image_label.setStyleSheet("background: transparent;")
        self.image_label.hide()

    def setup_workflow(self, main_window):
        """ 关联主窗口功能 """
        self.main_window = main_window

    def show_text(self, content: str):
        """ 显示文字内容 """
        self._current_mode = "text"
        self.text_label.setText(content)
        self.text_label.show()
        self.image_label.hide()
        self.update()

    def show_image(self, res: str):
        """ 显示处理后的图片 """
        if not res:
            return

        image_path, size = res.split("%%%")
        tex = max(eval(size.split(":")[1])) + 15  # 加一个边界距离把

        self._last_image_path = image_path
        self._current_mode = "image"

        # 加载并处理图片
        pixmap = QPixmap(image_path)
        if pixmap.isNull():
            self.show_text("无效的图片文件")
            return

        # 分步处理图片
        cropped = self._crop_center(pixmap, QSize(tex, tex))
        scaled = self._smart_scale(cropped, self.image_label.size())

        self.image_label.setAlignment(Qt.AlignCenter)
        self.image_label.setPixmap(scaled)
        self.image_label.show()
        self.text_label.hide()
        self.update()

    def _crop_center(self, pixmap: QPixmap, target: QSize) -> QPixmap:
        """ 裁剪图片中心区域 """
        source_size = pixmap.size()
        crop_size = QSize(min(source_size.width(), target.width()), min(source_size.height(), target.height()))

        width, height = (source_size.width() - crop_size.width()) // 2, (source_size.height() - crop_size.height()) // 2
        return pixmap.copy(QRect(QPoint(width, height), crop_size))

    def _smart_scale(self, pixmap: QPixmap, target: QSize) -> QPixmap:
        """ 智能缩放策略 """
        scaled = pixmap.scaled(
            target,
            Qt.KeepAspectRatioByExpanding,
            Qt.SmoothTransformation
        )
        return scaled

    def resizeEvent(self, event):
        """ 处理尺寸变化 """
        super().resizeEvent(event)

        # 更新子控件尺寸
        padding = 15
        content_rect = self.rect().adjusted(padding, padding, -padding, -padding)

        self.text_label.setGeometry(content_rect)
        self.image_label.setGeometry(self.rect())

        # 如果当前是图片模式需要重新缩放
        if self._current_mode == "image" and self._last_image_path:
            self.show_image(self._last_image_path)

    def mousePressEvent(self, event):
        """ 处理鼠标点击事件 """
        if self.isEnabled():  # 仅在启用状态下处理事件
            super().mousePressEvent(event)

            if event.button() == Qt.LeftButton:
                self.leftClicked.emit()
            elif event.button() == Qt.RightButton:
                self.rightClicked.emit()

    def clear_content(self):
        """ 重置为初始状态 """
        self.show_text(self._placeholder_text)
        self._last_image_path = ""

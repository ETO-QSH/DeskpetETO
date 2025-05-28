import os.path
import shutil
import subprocess
import sys
import json
from pathlib import Path

from PyQt5 import QtCore, QtWidgets
from PyQt5.QtCore import Qt, pyqtSignal, QSize, QRect, QPoint, QThread, pyqtSlot
from PyQt5.QtGui import QFont, QPixmap
from PyQt5.QtWidgets import QWidget, QApplication, QButtonGroup, QStackedWidget, QFileDialog, QLabel

from DeskpetETO.ImageDropWidget import ImageDropWidget
from DeskpetETO.FileListSettingCard import FileListSettingCard
from DeskpetETO.FilesDropWidget import FilesDropWidget
from DeskpetETO.JsonMerger import JsonMerger
from DeskpetETO.MessageBox import CustomMessageBox
from DeskpetETO.StateToolTip import StateToolTip

from qfluentwidgets import (
    SimpleCardWidget, ToolButton, PrimaryPushButton, FluentIcon, ToolTipFilter, ToolTipPosition, TogglePushButton,
    HorizontalPipsPager, ConfigItem, InfoBar, InfoBarPosition, ElevatedCardWidget
)


class Ui_UserCard(object):
    def setupUi(self, Form):
        Form.setObjectName("UserCard")
        Form.resize(450, 720)
        Form.setFixedSize(450, 720)  # 合并尺寸限制

        # 初始化状态监控
        self.is_full = None
        self.is_same = None

        # 主卡片容器
        self.SimpleCardWidget = SimpleCardWidget(Form)
        self.SimpleCardWidget.setGeometry(0, 0, 450, 720)

        # 内容容器
        self.content_widget = QWidget(self.SimpleCardWidget)
        self.content_widget.setGeometry(15, 10, 420, 700)

        # 主垂直布局
        main_layout = QtWidgets.QVBoxLayout(self.content_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)

        # ===== 顶部弹性空间 =====
        main_layout.addStretch(1)

        # ===== 设置卡片区域 =====
        card_layout = QtWidgets.QHBoxLayout()
        card_layout.setContentsMargins(0, 0, 0, 0)
        card_layout.addStretch(1)

        # 文件列表设置卡
        self.fileListCard = FileListSettingCard(
            ConfigItem("ModelFiles", "Path", ""),
            title="选择模型文件",
            content="添加 Spine 动画文件（*.skel, *.atlas, *.png）",
            directory="./"
        )
        self.fileListCard.setFixedSize(400, 240)
        self.fileListCard.stateFull.connect(self.on_state_full)
        self.fileListCard.sameName.connect(self.on_same_name)

        card_layout.addWidget(self.fileListCard)
        card_layout.addStretch(1)
        main_layout.addLayout(card_layout)

        # ===== 文件拖放区域 =====
        self.filesDropWidget = FilesDropWidget(file_types=[".png", ".skel", ".atlas"])
        self.filesDropWidget.bindFileListSettingCard(self.fileListCard)
        main_layout.addWidget(self.filesDropWidget)

        # ===== 中间间距 =====
        main_layout.addStretch(1)

        # ===== 切换区域 =====
        turn_layout = QtWidgets.QHBoxLayout()
        turn_layout.addStretch(3)

        # 左侧按钮垂直布局
        left_btn_layout = QtWidgets.QVBoxLayout()
        left_btn_layout.addStretch(1)

        self.dropUploadPushButton = TogglePushButton()
        self.dropUploadPushButton.setText("拖拽\n上传")
        self.dropUploadPushButton.setFont(QFont('萝莉体', 10))
        self.dropUploadPushButton.setFixedSize(60, 60)
        left_btn_layout.addWidget(self.dropUploadPushButton)
        left_btn_layout.addStretch(1)

        self.spinePreviewPushButton = TogglePushButton()
        self.spinePreviewPushButton.setText("模型\n预览")
        self.spinePreviewPushButton.setFont(QFont('萝莉体', 10))
        self.spinePreviewPushButton.setFixedSize(60, 60)
        left_btn_layout.addWidget(self.spinePreviewPushButton)
        left_btn_layout.addStretch(1)

        self.headChoicePushButton = TogglePushButton()
        self.headChoicePushButton.setText("头像\n选择")
        self.headChoicePushButton.setFont(QFont('萝莉体', 10))
        self.headChoicePushButton.setFixedSize(60, 60)
        left_btn_layout.addWidget(self.headChoicePushButton)
        left_btn_layout.addStretch(1)

        # 分页器容器
        self.pagerContainer = QtWidgets.QWidget()
        self.pagerContainer.setFixedSize(320, 320)

        # 使用堆叠控件代替HorizontalFlipView
        self.stack = QStackedWidget(self.pagerContainer)
        self.stack.setFixedSize(280, 280)
        self.stack.move(20, 10)

        # 添加三个自定义控件
        self.widget1 = self.filesDropWidget
        self.widget2 = ClickableElevatedCardWidget()
        self.widget3 = ImageDropWidget("./resource/404.png")
        self.stack.addWidget(self.widget1)
        self.stack.addWidget(self.widget2)
        self.stack.addWidget(self.widget3)

        # 绑定信号到打印函数
        self.widget2.setup_workflow(self)  # 关联工作流
        self.widget2.leftClicked.connect(self.elevatedCardLeft)
        self.widget2.rightClicked.connect(self.elevatedCardRight)

        # 分页指示器
        self.pager = HorizontalPipsPager(self.pagerContainer)
        self.pager.setFixedSize(40, 20)
        self.pager.move(120, 300)
        self.pager.setPageNumber(3)

        self.buttonGroup = QButtonGroup(exclusive=True)  # 设置按钮组
        self.dropUploadPushButton.setChecked(True)  # 初始激活

        # 信号连接
        self.buttonGroup.addButton(self.dropUploadPushButton, 0)
        self.buttonGroup.addButton(self.spinePreviewPushButton, 1)
        self.buttonGroup.addButton(self.headChoicePushButton, 2)

        self.buttonGroup.buttonToggled.connect(self.onButtonToggled)
        self.pager.currentIndexChanged.connect(self.onPagerChanged)
        self.stack.currentChanged.connect(self.onFlipViewChanged)

        # 组装切换区域
        turn_layout.addLayout(left_btn_layout)
        turn_layout.addStretch(1)
        turn_layout.addWidget(self.pagerContainer)
        turn_layout.addStretch(1)

        main_layout.addLayout(turn_layout)

        # ===== 底部间距 =====
        main_layout.addStretch(1)

        # ===== 底部按钮水平布局 =====
        bottom_layout = QtWidgets.QHBoxLayout()
        bottom_layout.setContentsMargins(0, 0, 0, 0)
        bottom_layout.addStretch(1)

        self.batchToolButton = ToolButton()
        self.batchToolButton.setFixedSize(45, 45)
        self.batchToolButton.setIcon(FluentIcon.ROBOT)
        self.batchToolButton.clicked.connect(self.select_and_validate_json_file)
        bottom_layout.addWidget(self.batchToolButton)
        bottom_layout.addStretch(1)

        self.batchToolButton.setToolTip('通过.json文件进行批处理')
        self.batchToolButton.setToolTipDuration(-1)
        self.batchToolButton.installEventFilter(
            ToolTipFilter(self.batchToolButton, showDelay=100, position=ToolTipPosition.TOP)
        )

        self.acceptPushButton = PrimaryPushButton()
        self.acceptPushButton.setFixedSize(216, 54)
        self.acceptPushButton.setText("保 存 至 模 型 库")
        self.acceptPushButton.setFont(QFont('萝莉体', 13))
        self.acceptPushButton.clicked.connect(self.gather_info)
        bottom_layout.addWidget(self.acceptPushButton)
        bottom_layout.addStretch(1)

        self.clearToolButton = ToolButton()
        self.clearToolButton.setFixedSize(45, 45)
        self.clearToolButton.setIcon(FluentIcon.CLOSE)
        self.clearToolButton.clicked.connect(self.clear_info)
        bottom_layout.addWidget(self.clearToolButton)
        bottom_layout.addStretch(1)

        main_layout.addLayout(bottom_layout)

        # ===== 底部弹性空间 =====
        main_layout.addStretch(1)

        QtCore.QMetaObject.connectSlotsByName(Form)

    def onButtonToggled(self, button, checked):
        """ 切换按钮事件 """
        if checked:
            index = self.buttonGroup.id(button)
            self.pager.setCurrentIndex(index)
            self.stack.setCurrentIndex(index)

    def onPagerChanged(self, index):
        """ 分页指示器变化 """
        self.buttonGroup.button(index).setChecked(True)
        self.stack.setCurrentIndex(index)

    def onFlipViewChanged(self, index):
        """ 图片视图变化 """
        self.pager.setCurrentIndex(index)

    def on_state_full(self, is_full):
        self.is_full = is_full

    def on_same_name(self, is_same):
        self.is_same = is_same

    def elevatedCardLeft(self):
        self.fileListCard._checkState()

        if self.is_full and self.is_same:
            files = self.fileListCard.files

            for path in files.values():
                if not Path(path).is_file():
                    self.error_infor("文件不存在", path)
                    return

            # 显示状态提示
            self.state_tooltip = StateToolTip('处理中，请稍后...', self.stack)
            self.state_tooltip.setFixedWidth(180)
            self.state_tooltip.setFixedHeight(36)
            self.state_tooltip.setToolTipDuration(-1)

            self.state_tooltip.titleLabel.setStyleSheet(
                """
                   QLabel {
                       font-family: 萝莉体;
                       font-size: 11pt;
                   }
                """
            )

            width, height = self.stack.width(), self.stack.height()
            w, h = self.state_tooltip.width(), self.state_tooltip.height()

            self.state_tooltip.move((width - w) // 2, (height - h) // 2)
            self.state_tooltip.show()

            # 禁用交互
            self._set_widgets_enabled(False)

            # 获取文件数据
            files = self.fileListCard.files

            # 启动工作线程
            self.worker = SpinePreviewThread(files["skel"], files["atlas"], files["png"])
            self.worker.finished.connect(
                lambda code, result_path: self._handle_preview_result(code, result_path)
            )
            self.worker.start()

        else:
            InfoBar.warning(
                title='Spine模型文件无效，请保证文件名一致',
                content="",
                orient=Qt.Horizontal,
                isClosable=True,
                position=InfoBarPosition.TOP,
                duration=3000,
                parent=self.content_widget
            )

    def _set_widgets_enabled(self, enabled):
        """ 设置控件启用状态 """
        self.widget2.setEnabled(enabled)
        self.fileListCard.setEnabled(enabled)

    @pyqtSlot(int, str)
    def _handle_preview_result(self, code, result_path):
        """ 处理预览结果 """
        # 关闭状态提示
        self.state_tooltip.close()

        # 恢复交互
        self._set_widgets_enabled(True)

        if code == 0 and os.path.exists(result_path):
            self.widget2.show_image(result_path)
        elif code == 1:
            self.error_infor("模型解析失败")
        elif code == -1:
            self.error_infor("发生其他错误")

    def elevatedCardRight(self):
        print("elevatedCardRight")

    def gather_info(self):
        self.fileListCard._checkState()

        if self.is_full and self.is_same:
            self.json_merger = JsonMerger()
            agent_list = self.json_merger.data.keys()
            skin_list = []
            brand_list = self.json_merger.brands
            model_list = ["基建", "正面", "背面"]

            w = CustomMessageBox(
                self.SimpleCardWidget, agent_list, skin_list, brand_list, model_list
            )
            w.yesButton.setText("确认")
            w.cancelButton.setText("取消")

            if w.exec():
                print(w.getInputs())
                paths = self.fileListCard.files
                paths["head"] = self.widget3.image_path
                self.clear_info()
                print(paths)

        else:
            InfoBar.warning(
                title='请添加能够通过预览的完整Spine模型文件',
                content="",
                orient=Qt.Horizontal,
                isClosable=True,
                position=InfoBarPosition.TOP,
                duration=3000,
                parent=self.content_widget
            )

    def clear_info(self):
        self.fileListCard.clear_files()
        self.widget2.clear_content()
        self.widget3._load_default_image()

    def select_and_validate_json_file(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self.content_widget, "选择 JSON 文件", "./", "JSON 文件 (*.json)"
        )

        if not file_path:
            return

        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
        except Exception as e:
            self.error_infor("文件读取失败", str(e))
            return

        if not isinstance(data, list):
            self.error_infor("文件结构错误", "请验证传入的文件格式满足规范")
            return

        for item in data:
            if not isinstance(item, dict):
                self.error_infor("文件结构错误", "请验证传入的文件格式满足规范")
                return

            required_keys = {"skel", "atlas", "png", "head", "agent", "skin", "brand", "model"}
            if not required_keys.issubset(item.keys()):
                self.error_infor("缺少必要的键", f"{required_keys - item.keys()}")
                return

            for key in ["agent", "skin", "brand", "model"]:
                if not isinstance(item[key], str):
                    self.error_infor("数据类型错误", f"{key} 的值应是字符串")

            for key, path in item.items():
                if key in ["skel", "atlas", "png", "head"]:
                    if not Path(path).is_file():
                        self.error_infor("文件不存在", path)
                        return
                    if key in ["skel", "atlas", "png"]:
                        expected_ext = f".{key}"
                        if not Path(path).suffix.lower() == expected_ext:
                            self.error_infor("扩展名不匹配", f"{path} (应为 {expected_ext}")
                            return
                    elif key == "head":
                        if not Path(path).suffix.lower() in [".png", ".jpg", ".jpeg"]:
                            self.error_infor("头像文件类型错误", f"{path} (应为 .png, .jpg 或 .jpeg)")
                            return

            base_names = [Path(item["skel"]).stem, Path(item["atlas"]).stem, Path(item["png"]).stem]
            if not all(name == base_names[0] for name in base_names):
                self.error_infor("模型文件错误", "skel, atlas, png 文件名不一致")
                return

            code, _ = spine_preview(item["skel"], item["atlas"], item["png"])

            if code == 1:
                self.error_infor("模型解析失败")
            elif code == -1:
                self.error_infor("发生其他错误")

        print(data)

    def error_infor(self, title, content=""):
        InfoBar.error(
            title=title,
            content=content,
            orient=Qt.Horizontal,
            isClosable=True,
            position=InfoBarPosition.TOP,
            duration=3000,
            parent=self.content_widget
        )


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

    def show_image(self, image_path: str):
        """ 显示处理后的图片 """
        if not image_path:
            return

        self._last_image_path = image_path
        self._current_mode = "image"

        # 加载并处理图片
        pixmap = QPixmap(image_path)
        if pixmap.isNull():
            self.show_text("无效的图片文件")
            return

        # 分步处理图片
        cropped = self._crop_center(pixmap, QSize(432, 432))
        scaled = self._smart_scale(cropped, self.image_label.size())

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
            target * self.devicePixelRatioF(),
            Qt.KeepAspectRatioByExpanding,
            Qt.SmoothTransformation
        )
        scaled.setDevicePixelRatio(self.devicePixelRatioF())
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


class SpinePreviewThread(QThread):
    """ 骨骼预览生成线程 """
    finished = pyqtSignal(int, str)  # (状态码, 结果路径)

    def __init__(self, skel_path, atlas_path, png_path):
        super().__init__()
        self.skel_path = skel_path
        self.atlas_path = atlas_path
        self.png_path = png_path

    def run(self):
        try:
            os.makedirs("./output/temp", exist_ok=True)
            path = os.path.join("./output/temp", Path(self.skel_path).stem)

            files = []
            for file, suffix in [(self.skel_path, ".skel"), (self.atlas_path, ".atlas"), (self.png_path, ".png")]:
                shutil.copyfile(file, path + suffix)
                files.append(path + suffix)

            result = subprocess.run(["./preview/preview.exe", *files[:2], path + "_output.png"])

            for file in files:
                os.remove(file)

            if result.returncode == 0:
                self.finished.emit(0, path + "_output.png")
            else:
                self.finished.emit(1, "模型解析失败")

        except Exception as e:
            self.finished.emit(-1, str(e))
            print(e)


def spine_preview(skel, atlas, png):
    os.makedirs("./output/temp", exist_ok=True)
    path = os.path.join("./output/temp", Path(skel).stem)

    files = []
    for file, suffix in [(skel, ".skel"), (atlas, ".atlas"), (png, ".png")]:
        shutil.copyfile(file, path + suffix)
        files.append(path + suffix)

    result = subprocess.run(["./preview/preview.exe", *files[:2], path + "_output.png"])

    for file in files:
        os.remove(file)

    if result.returncode == 0:
        return 0, path + "_output.png"
    else:
        return 1, "模型解析失败"


class PreviewWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.ui = Ui_UserCard()
        self.ui.setupUi(self)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = PreviewWindow()
    window.show()
    sys.exit(app.exec_())

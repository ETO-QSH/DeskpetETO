import os
import sys
import copy
import json
import shutil
from pathlib import Path

from PyQt5 import QtCore, QtWidgets
from PyQt5.QtCore import Qt, pyqtSlot
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import QWidget, QApplication, QButtonGroup, QStackedWidget, QFileDialog

from DeskpetETO.ClickableElevated import ClickableElevatedCardWidget
from DeskpetETO.ImageDropWidget import ImageDropWidget
from DeskpetETO.FileListSettingCard import FileListSettingCard
from DeskpetETO.FilesDropWidget import FilesDropWidget
from DeskpetETO.JsonMerger import JsonMerger
from DeskpetETO.MessageBox import CustomMessageBox
from DeskpetETO.StateToolTip import StateToolTip
from DeskpetETO.ToolCalling import SpinePreviewThread, spine_preview

from qfluentwidgets import (
    SimpleCardWidget, ToolButton, PrimaryPushButton, FluentIcon, ToolTipFilter, ToolTipPosition,
    TogglePushButton, HorizontalPipsPager, ConfigItem, InfoBar, InfoBarPosition
)


class Ui_UserCard(object):
    def setupUi(self, Form):
        Form.setObjectName("UserCard")
        Form.resize(450, 720)
        Form.setFixedSize(450, 720)  # 合并尺寸限制

        self.json_merger = JsonMerger()  # 初始化管理器

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
                title='Spine模型文件无效，请保证文件名一致，且并未更改文件名',
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

        if code == 0 and os.path.exists(result_path.split("%%%")[0]):
            self.widget2.show_image(result_path)
        else:
            self.error_infor(result_path)

    def elevatedCardRight(self):
        print("elevatedCardRight")

    def gather_info(self):
        self.fileListCard._checkState()

        if self.is_full and self.is_same:
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
                infos = w.getInputs()
                paths = self.fileListCard.files
                paths["head"] = self.widget3.image_path

                user_dir = "./user_saves"
                os.makedirs(user_dir, exist_ok=True)

                head_path = os.path.join(user_dir, infos['干员代号'], "head", f"{infos['皮肤名称']}.png")
                os.makedirs(os.path.join("resource", Path(head_path).parent), exist_ok=True)
                shutil.copy(self.widget3.image_path, os.path.join("resource", head_path))

                spine_paths = copy.deepcopy(paths)
                for item in ["png", "skel", "atlas"]:
                    spine_path = os.path.join(
                        user_dir, infos['干员代号'], "spine", infos['皮肤名称'], infos['模型分类'], Path(paths[item]).name
                    )
                    os.makedirs(os.path.join("resource", Path(spine_path).parent), exist_ok=True)
                    shutil.copy(paths[item], os.path.join("resource", spine_path))
                    spine_paths[item] = spine_path

                self.json_merger.save_user_saves({
                    infos['干员代号']: {
                        infos['皮肤名称']: {
                            "head": head_path,
                            infos['模型分类']: {
                                "png": spine_paths["png"],
                                "skel": spine_paths["skel"],
                                "atlas": spine_paths["atlas"]
                            }
                        }
                    }
                })

                self.json_merger.save_user_brands({infos["品牌分类"]: [infos['皮肤名称']]})
                self.clear_info()

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

            code, info = spine_preview(item["skel"], item["atlas"], item["png"])

            if code == 0:
                self.save_item_info(item)
            elif code == -1:
                self.error_infor("发生其他错误", f"{item['agent']}-{item['skin']}-{item['model']}")
                continue
            else:
                self.error_infor("模型解析失败", info)
                continue

        InfoBar.success(
            title='SUCCESS',
            content="已添加所有有效模型！",
            orient=Qt.Horizontal,
            isClosable=True,
            position=InfoBarPosition.TOP,
            duration=3000,
            parent=self.content_widget
        )

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

    def save_item_info(self, item):
        user_dir = "./user_saves"
        os.makedirs(user_dir, exist_ok=True)

        head_path = os.path.join(user_dir, item['name'], "head", f"{item['skin']}.png")
        os.makedirs(os.path.join("resource", Path(head_path).parent), exist_ok=True)
        shutil.copy(item['head'], os.path.join("resource", head_path))

        spine_paths = {"png": None, "skel": None, "atlas": None, }
        for it in spine_paths.keys():
            spine_path = os.path.join(
                user_dir, item['agent'], "spine", item['skin'], item['model'], Path(item[it]).name
            )
            os.makedirs(os.path.join("resource", Path(spine_path).parent), exist_ok=True)
            shutil.copy(item[it], os.path.join("resource", spine_path))
            spine_paths[it] = spine_path

        self.json_merger.save_user_saves({
            item['agent']: {
                item['skin']: {
                    "head": head_path,
                    item['model']: {
                        "png": item["png"],
                        "skel": item["skel"],
                        "atlas": item["atlas"]
                    }
                }
            }
        })

        self.json_merger.save_user_brands({item["brand"]: [item['skin']]})


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

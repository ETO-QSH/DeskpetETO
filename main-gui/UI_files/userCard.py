import sys
from PyQt5 import QtCore, QtWidgets
from PyQt5.QtGui import QFont, QPixmap
from PyQt5.QtWidgets import QWidget, QApplication, QButtonGroup, QStackedWidget

from DeskpetETO.ImageDropWidget import ImageDropWidget


class Ui_UserCard(object):
    def setupUi(self, Form):
        Form.setObjectName("Form")
        Form.resize(480, 720)
        Form.setFixedSize(480, 720)  # 合并尺寸限制

        # 主卡片容器
        self.SimpleCardWidget = SimpleCardWidget(Form)
        self.SimpleCardWidget.setGeometry(0, 0, 480, 720)

        # 内容容器
        content_widget = QWidget(self.SimpleCardWidget)
        content_widget.setGeometry(30, 45, 422, 651)

        # 主垂直布局
        main_layout = QtWidgets.QVBoxLayout(content_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)

        # ===== 顶部弹性空间 =====
        main_layout.addStretch(1)

        # ===== 设置卡片 =====
        self.settingSimpleCardWidget = SimpleCardWidget()
        self.settingSimpleCardWidget.setFixedSize(420, 210)
        main_layout.addWidget(self.settingSimpleCardWidget)

        # ===== 中间间距 =====
        main_layout.addStretch(2)

        # ===== 切换区域 =====
        turn_layout = QtWidgets.QHBoxLayout()
        turn_layout.addStretch(3)

        # 左侧按钮垂直布局
        left_btn_layout = QtWidgets.QVBoxLayout()
        left_btn_layout.addStretch(1)

        self.dropUploadPushButton = TogglePushButton()
        self.dropUploadPushButton.setText("拖拽\n上传")
        self.dropUploadPushButton.setFont(QFont('萝莉体', 8))
        self.dropUploadPushButton.setFixedSize(60, 60)
        left_btn_layout.addWidget(self.dropUploadPushButton)
        left_btn_layout.addStretch(1)

        self.spinePreviewPushButton = TogglePushButton()
        self.spinePreviewPushButton.setText("模型\n预览")
        self.spinePreviewPushButton.setFont(QFont('萝莉体', 8))
        self.spinePreviewPushButton.setFixedSize(60, 60)
        left_btn_layout.addWidget(self.spinePreviewPushButton)
        left_btn_layout.addStretch(1)

        self.headChoicePushButton = TogglePushButton()
        self.headChoicePushButton.setText("头像\n选择")
        self.headChoicePushButton.setFont(QFont('萝莉体', 8))
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
        self.widget1 = ImageDropWidget(QPixmap("../resource/404.png"))
        self.widget2 = ImageDropWidget(QPixmap("../resource/404.png"))
        self.widget3 = ImageDropWidget(QPixmap("../resource/404.png"))
        self.stack.addWidget(self.widget1)
        self.stack.addWidget(self.widget2)
        self.stack.addWidget(self.widget3)

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
        self.acceptPushButton.setFont(QFont('萝莉体', 10))
        bottom_layout.addWidget(self.acceptPushButton)
        bottom_layout.addStretch(1)

        self.clearToolButton = ToolButton()
        self.clearToolButton.setFixedSize(45, 45)
        self.clearToolButton.setIcon(FluentIcon.CLOSE)
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


from qfluentwidgets import SimpleCardWidget, ToolButton, PrimaryPushButton, FluentIcon, ToolTipFilter, \
    ToolTipPosition, TogglePushButton, HorizontalFlipView, HorizontalPipsPager


class PreviewWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.ui = Ui_UserCard()
        self.ui.setupUi(self)
        self.setStyleSheet("""
            background: #aaaaaa;
            SimpleCardWidget {
                border: 2px dashed rgba(0,0,0,0.2);
                border-radius: 8px;
            }
        """)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = PreviewWindow()
    window.show()
    sys.exit(app.exec_())

import os
import json

from PyQt5 import QtWidgets, QtCore, QtGui
from PyQt5.QtGui import QPixmap
from PyQt5.QtCore import Qt, QSize, pyqtSignal
from PyQt5.QtWidgets import QWidget

from qfluentwidgets import (
    ElevatedCardWidget, BodyLabel, ToolButton, FluentIcon, Theme, isDarkTheme, ImageLabel,
    EditableComboBox, TransparentToolButton, PrimaryPushButton, Flyout, InfoBarIcon, FlyoutAnimationType
)


class CustomCard(ElevatedCardWidget):
    btnClicked = pyqtSignal(int)  # 按钮索引信号
    dragStarted = pyqtSignal(QtWidgets.QWidget)  # 新增拖拽信号

    def __init__(self, card_id, parent=None):
        super().__init__(parent)
        self.card_id = card_id
        self._opacity = 1.0  # 使用成员变量存储透明度状态
        self.setAttribute(Qt.WA_TranslucentBackground)  # 启用透明背景支持

        self._init_ui()
        self._init_style()

        self.set_button_icons([
            FluentIcon.COPY,
            FluentIcon.MOVE,
            FluentIcon.RINGER,
            FluentIcon.DELETE
        ])

        # 为移动按钮添加拖拽支持
        self.buttons[1].installEventFilter(self)  # 索引1是移动按钮

    def _init_ui(self):
        self.setFixedSize(520, 120)
        self.mainWidget = QtWidgets.QWidget(self)
        self.mainWidget.setGeometry(0, 0, 520, 120)

        # 使用水平布局
        self.hLayout = QtWidgets.QHBoxLayout(self.mainWidget)
        self.hLayout.setContentsMargins(16, 8, 16, 8)
        self.hLayout.setSpacing(16)

        # 图标区域
        self.iconBadge = ImageLabel()
        self.iconBadge.setFixedSize(108, 108)
        self.iconBadge.setBorderRadius(16, 16, 16, 16)
        self.hLayout.addWidget(self.iconBadge)

        # 添加分隔线
        self.separator = QtWidgets.QFrame()
        self.separator.setFrameShape(QtWidgets.QFrame.VLine)
        self.separator.setFrameShadow(QtWidgets.QFrame.Sunken)
        self.separator.setStyleSheet("color: #606060; border: 0px")
        self.separator.setFixedWidth(16)
        self.hLayout.addWidget(self.separator)

        # 文字区域
        self.textLayout = QtWidgets.QVBoxLayout()
        self.bodyLabel = BodyLabel()
        self.bodyLabel.setAlignment(Qt.AlignLeft | Qt.AlignVCenter)

        self.textLayout.addStretch()  # 顶部弹簧
        self.textLayout.addWidget(self.bodyLabel)
        self.textLayout.addStretch()  # 底部弹簧

        self.textLayout.addStretch()
        self.hLayout.addLayout(self.textLayout)

        # 按钮区域
        self.btnLayout = QtWidgets.QGridLayout()
        self.btnLayout.setContentsMargins(0, 0, 0, 0)  # 移除边距
        self.buttons = []
        for i in range(4):
            btn = ToolButton()
            btn.setFixedSize(48, 48)
            btn.setIconSize(QSize(32, 32))
            btn.setStyleSheet("""
                ToolButton {
                    border-radius: 16px;
                    padding: 8px;
                }
                ToolButton:hover {
                    background-color: rgba(0,0,0,0.05);
                }
            """)
            btn.clicked.connect(lambda _, idx=i: self.btnClicked.emit(idx))
            self.buttons.append(btn)
            self.btnLayout.addWidget(btn, i // 2, i % 2)

        # 在按钮布局外包裹一个容器调整边距
        btn_container = QtWidgets.QWidget()
        btn_container.setLayout(self.btnLayout)
        self.hLayout.addWidget(btn_container, stretch=0, alignment=Qt.AlignRight)
        self.hLayout.setSpacing(0)  # 调整主布局间距

    def _init_style(self):
        # 根据主题设置图标颜色
        self.iconBadge.setProperty('isDark', isDarkTheme())

        # 设置字体
        font = self.bodyLabel.font()
        font.setPointSize(11)
        self.bodyLabel.setFont(font)

    def set_image(self, image, size=96):
        """设置图标
        :param image: 图标路径
        :param size: 图标尺寸
        """
        self.iconBadge.setPixmap(QPixmap(image).scaled(size, size, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        self.iconBadge.setBaseSize(QSize(size, size))

    def set_button_icons(self, icons):
        """设置按钮图标
        :param icons: 包含4个FluentIcon的列表
        """
        theme = Theme.DARK if isDarkTheme() else Theme.LIGHT
        for btn, fluent_icon in zip(self.buttons, icons):
            btn.setIcon(fluent_icon.icon(theme))

    def set_text(self, agent, skin, model):
        """设置文本内容"""
        text = f"<b>• {agent}</b>"
        text += f"<br><span style='color:#000;'>• {skin}</span>"
        text += f"<br><span style='color:#666;'>• {model}</span>"
        self.bodyLabel.setText(text)

    def eventFilter(self, obj, event):
        # 处理移动按钮的拖拽事件
        if hasattr(self, 'buttons') and obj == self.buttons[1]:
            if event.type() == QtCore.QEvent.MouseButtonPress:
                self.mousePressPos = event.pos()
            elif event.type() == QtCore.QEvent.MouseMove:
                if hasattr(self, 'mousePressPos'):
                    if (event.pos() - self.mousePressPos).manhattanLength() > 10:
                        self.startDrag()
                        return True
            elif event.type() == QtCore.QEvent.MouseButtonRelease:
                if hasattr(self, 'mousePressPos'):
                    del self.mousePressPos
        return super().eventFilter(obj, event)

    def startDrag(self):
        self.setOpacity(2/3)  # 别问为什么写那么多次

        # 创建拖拽预览图
        pixmap = self.grab()
        ghost_pixmap = QtGui.QPixmap(pixmap.size())
        ghost_pixmap.fill(Qt.transparent)
        painter = QtGui.QPainter(ghost_pixmap)
        painter.setOpacity(2/3)  # 问就是说希腊奶
        painter.drawPixmap(0, 0, pixmap)
        painter.end()

        drag = QtGui.QDrag(self)
        drag.setPixmap(ghost_pixmap)  # 使用半透明预览图

        # 创建拖拽对象
        drag = QtGui.QDrag(self)
        mime = QtCore.QMimeData()
        mime.setData("application/x-card", self.card_id.encode())  # 需要传递卡片ID
        drag.setMimeData(mime)

        # 创建拖拽预览图像
        pixmap = self.grab()
        drag.setPixmap(pixmap)
        drag.setHotSpot(QtCore.QPoint(pixmap.width()//2, pixmap.height()//2))

        # 发射拖拽开始信号
        self.dragStarted.emit(self)
        drag.exec_(QtCore.Qt.MoveAction)

    def setOpacity(self, value):
        """设置卡片透明度 (0.0~1.0)"""
        self._opacity = max(0.0, min(1.0, value))
        self.update()

    def paintEvent(self, event):
        """自定义绘制实现透明度效果"""
        painter = QtGui.QPainter(self)
        painter.setOpacity(self._opacity)
        super().paintEvent(event)


class AddCard(QWidget):
    cardRequested = QtCore.pyqtSignal(str, str, str, str)  # agent, skin, model, image_path
    switchToFilter = QtCore.pyqtSignal()  # 新增切换信号

    def __init__(self, parent=None):
        super().__init__(parent)
        self.resize(520, 120)
        self.data = {}  # 存储加载的JSON数据
        self.init_ui()
        self.load_combo_data()

    def init_ui(self):
        # 主卡片容器
        self.card = QWidget(self)
        self.card.setGeometry(0, 0, 520, 120)

        # 垂直布局（主布局）
        self.verticalLayout = QtWidgets.QVBoxLayout(self.card)
        self.verticalLayout.addStretch()
        self.verticalLayout.setContentsMargins(28, 8, 16, 16)

        # 水平布局（核心调整部分）
        self.horizontalLayout = QtWidgets.QHBoxLayout()

        # ================== 第一组（干员选择） ==================

        # 下拉框
        self.combo_agent = EditableComboBox()
        self.combo_agent.setPlaceholderText("选择干员")
        self.combo_agent.setFixedSize(135, 45)
        self.combo_agent.currentTextChanged.connect(self.on_agent_changed)
        self.combo_agent.setMaxVisibleItems(6)
        self.horizontalLayout.addWidget(self.combo_agent)

        # 第一个弹簧和徽章
        self.horizontalLayout.addStretch()
        self.badge1 = TransparentToolButton(FluentIcon.CHEVRON_RIGHT)
        self.badge1.setFixedSize(25, 25)
        self.horizontalLayout.addWidget(self.badge1, 0, Qt.AlignCenter)
        self.horizontalLayout.addStretch()

        # ================== 第二组（皮肤选择） ==================

        self.combo_skin = EditableComboBox()
        self.combo_skin.setPlaceholderText("选择皮肤")
        self.combo_skin.setFixedSize(135, 45)
        self.combo_skin.setEnabled(False)  # 初始禁用
        self.combo_skin.currentTextChanged.connect(self.on_skin_changed)
        self.combo_skin.setMaxVisibleItems(6)
        self.horizontalLayout.addWidget(self.combo_skin)

        # 第二个弹簧和徽章
        self.horizontalLayout.addStretch()
        self.badge2 = TransparentToolButton(FluentIcon.CHEVRON_RIGHT)
        self.badge2.setFixedSize(25, 25)
        self.horizontalLayout.addWidget(self.badge2, 0, Qt.AlignCenter)
        self.horizontalLayout.addStretch()

        # ================== 第三组（模型选择） ==================

        self.combo_model = EditableComboBox()
        self.combo_model.setPlaceholderText("选择模型")
        self.combo_model.setFixedSize(135, 45)
        self.combo_model.setEnabled(False)  # 初始禁用
        self.combo_model.setMaxVisibleItems(6)
        self.horizontalLayout.addWidget(self.combo_model)

        # ================== 添加按钮 ==================
        btn_container = QWidget()
        btn_layout = QtWidgets.QHBoxLayout(btn_container)
        btn_layout.setContentsMargins(0, 0, 0, 0)
        btn_layout.setSpacing(8)

        # 重载按钮
        self.btn_reload = ToolButton()
        self.btn_reload.setIcon(FluentIcon.SYNC)
        self.btn_reload.setFixedSize(80, 40)
        self.btn_reload.clicked.connect(self.load_combo_data)

        # 添加按钮
        self.btn_add = PrimaryPushButton("添加卡片")
        self.btn_add.setIcon(FluentIcon.ADD_TO)
        self.btn_add.setFixedSize(240, 45)
        self.btn_add.clicked.connect(self.on_add_clicked)

        # 清空按钮
        self.btn_filter = ToolButton()
        self.btn_filter.setIcon(FluentIcon.FILTER)
        self.btn_filter.setFixedSize(80, 40)
        self.btn_filter.clicked.connect(self.turn_2_filter)

        # 按钮布局
        btn_layout.addStretch()
        btn_layout.addWidget(self.btn_reload)
        btn_layout.addSpacing(15)
        btn_layout.addWidget(self.btn_add)
        btn_layout.addSpacing(15)
        btn_layout.addWidget(self.btn_filter)
        btn_layout.addStretch()

        # ================== 最终布局组装 ==================
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.verticalLayout.addStretch()
        self.verticalLayout.addWidget(btn_container)

    def turn_2_filter(self):
        self.switchToFilter.emit()

    def clear_selections(self):
        """清空所有选择"""
        self.combo_agent.setCurrentIndex(-1)
        self.combo_skin.setCurrentIndex(-1)
        self.combo_model.setCurrentIndex(-1)
        self.combo_skin.setEnabled(False)
        self.combo_model.setEnabled(False)

    def load_combo_data(self):
        """加载JSON数据并初始化干员列表"""
        self.clear_selections()
        try:
            with open(r".\resource\saves.json", "r", encoding="utf-8") as f:
                self.data = json.load(f)
            agents = list(self.data.keys())
            self.combo_agent.addItems(agents)
            self.combo_agent.setCurrentIndex(-1)
        except Exception as e:
            print(f"加载JSON数据失败: {e}")

    def on_agent_changed(self, agent):
        """干员选择变化事件"""
        self.combo_skin.clear()
        self.combo_model.clear()

        if not agent or agent not in self.data:
            self.combo_skin.setEnabled(False)
            self.combo_model.setEnabled(False)
            return

        # 填充皮肤列表
        skins = list(self.data[agent].keys())
        self.combo_skin.setEnabled(True)
        self.combo_skin.addItems(skins)

        # 动态设置可见项数
        skin_count = len(skins)
        self.combo_skin.setMaxVisibleItems(skin_count if skin_count <= 6 else 6)

        # 设置默认皮肤
        if "默认" in skins:
            self.combo_skin.setCurrentText("默认")
            self.on_skin_changed("默认")
        elif skins:
            self.combo_skin.setCurrentIndex(0)
            self.on_skin_changed(self.combo_skin.getCurrentText())

    def on_skin_changed(self, skin):
        """皮肤选择变化事件"""
        self.combo_model.clear()
        agent = self.combo_agent.currentText()

        if not agent or not skin or agent not in self.data or skin not in self.data[agent]:
            self.combo_model.setEnabled(False)
            return

        # 获取模型列表（排除head条目）
        models = [k for k in self.data[agent][skin].keys() if k != "head"]
        if not models:
            self.combo_model.setEnabled(False)
            return

        self.combo_model.setEnabled(True)
        self.combo_model.addItems(models)

        # 动态设置可见项数
        model_count = len(models)
        self.combo_model.setMaxVisibleItems(model_count if model_count <= 6 else 6)

        # 设置默认模型
        if "基建" in models:
            self.combo_model.setCurrentText("基建")
        elif models:
            self.combo_model.setCurrentIndex(0)

    def on_add_clicked(self):
        """添加卡片事件"""
        agent = self.combo_agent.currentText()
        skin = self.combo_skin.currentText()
        model = self.combo_model.currentText()

        if not all([agent, skin, model]):
            self.show_warning_flyout()
            return

        head_path = self.data[agent][skin].get("head", "")
        image_path = os.path.join("./resource", head_path)
        if not os.path.exists(image_path):
            self.show_error_flyout()
            return

        # 发射信号而不是直接操作UI
        self.cardRequested.emit(agent, skin, model, image_path)
        self.clear_selections()

    def show_warning_flyout(self):
        Flyout.create(
            icon=InfoBarIcon.WARNING,
            title='Warning!',
            content="完整填写所有选项喵 ~",
            target=self.btn_add,
            parent=self,
            aniType=FlyoutAnimationType.PULL_UP,
            isClosable=True
        )

    def show_error_flyout(self):
        Flyout.create(
            icon=InfoBarIcon.ERROR,
            title='Error!',
            content="路径错掉了喵 ~",
            target=self.btn_add,
            parent=self,
            aniType=FlyoutAnimationType.PULL_UP,
            isClosable=True
        )


class FilterCard(QWidget):
    switchToAdd = QtCore.pyqtSignal()  # 新增切换信号

    def __init__(self, parent=None):
        super().__init__(parent)
        self.resize(520, 120)
        self.data = {}  # 存储加载的JSON数据
        self.init_ui()
        self.load_combo_data()
        self.load_brands()
        self.combo_model.addItems(["基建", "正面", "背面"])
        self.combo_model.setCurrentIndex(-1)
        self.clear_selections()

    def init_ui(self):
        # 主卡片容器
        self.card = QWidget(self)
        self.card.setGeometry(0, 0, 520, 120)

        # 垂直布局（主布局）
        self.verticalLayout = QtWidgets.QVBoxLayout(self.card)
        self.verticalLayout.addStretch()
        self.verticalLayout.setContentsMargins(28, 8, 16, 16)

        # 水平布局（核心调整部分）
        self.horizontalLayout = QtWidgets.QHBoxLayout()

        # ================== 第一组（干员选择） ==================

        # 下拉框
        self.combo_agent = EditableComboBox()
        self.combo_agent.setPlaceholderText("选择干员")
        self.combo_agent.setFixedSize(135, 45)
        self.combo_agent.setMaxVisibleItems(6)
        self.horizontalLayout.addWidget(self.combo_agent)

        # 第一个弹簧和徽章
        self.horizontalLayout.addStretch()
        self.badge1 = TransparentToolButton(FluentIcon.IOT)
        self.badge1.setFixedSize(25, 25)
        self.horizontalLayout.addWidget(self.badge1, 0, Qt.AlignCenter)
        self.horizontalLayout.addStretch()

        # ================== 第二组（皮肤选择） ==================

        self.combo_skin = EditableComboBox()
        self.combo_skin.setPlaceholderText("选择品牌")
        self.combo_skin.setFixedSize(135, 45)
        self.combo_skin.setMaxVisibleItems(6)
        self.horizontalLayout.addWidget(self.combo_skin)

        # 第二个弹簧和徽章
        self.horizontalLayout.addStretch()
        self.badge2 = TransparentToolButton(FluentIcon.IOT)
        self.badge2.setFixedSize(25, 25)
        self.horizontalLayout.addWidget(self.badge2, 0, Qt.AlignCenter)
        self.horizontalLayout.addStretch()

        # ================== 第三组（模型选择） ==================

        self.combo_model = EditableComboBox()
        self.combo_model.setPlaceholderText("选择模型")
        self.combo_model.setFixedSize(135, 45)
        self.combo_model.setMaxVisibleItems(6)
        self.horizontalLayout.addWidget(self.combo_model)

        # ================== 添加按钮 ==================
        btn_container = QWidget()
        btn_layout = QtWidgets.QHBoxLayout(btn_container)
        btn_layout.setContentsMargins(0, 0, 0, 0)
        btn_layout.setSpacing(8)

        # 清空按钮
        self.btn_clear = ToolButton()
        self.btn_clear.setIcon(FluentIcon.DELETE)
        self.btn_clear.setFixedSize(80, 40)
        self.btn_clear.clicked.connect(self.clear_filter_cards)

        # 筛选按钮
        self.btn_filter = PrimaryPushButton("筛选卡片")
        self.btn_filter.setIcon(FluentIcon.FILTER)
        self.btn_filter.setFixedSize(240, 45)
        self.btn_filter.clicked.connect(self.on_filter_clicked)

        # 添加按钮
        self.btn_add = ToolButton()
        self.btn_add.setIcon(FluentIcon.ADD_TO)
        self.btn_add.setFixedSize(80, 40)
        self.btn_add.clicked.connect(self.turn_2_add)

        # 按钮布局
        btn_layout.addStretch()
        btn_layout.addWidget(self.btn_clear)
        btn_layout.addSpacing(15)
        btn_layout.addWidget(self.btn_filter)
        btn_layout.addSpacing(15)
        btn_layout.addWidget(self.btn_add)
        btn_layout.addStretch()

        # ================== 最终布局组装 ==================
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.verticalLayout.addStretch()
        self.verticalLayout.addWidget(btn_container)

    def turn_2_add(self):
        self.switchToAdd.emit()

    def clear_filter_cards(self):
        pass

    def on_filter_clicked(self):
        pass

    def clear_selections(self):
        """清空所有选择"""
        self.combo_agent.setCurrentIndex(-1)
        self.combo_skin.setCurrentIndex(-1)
        self.combo_model.setCurrentIndex(-1)

    def load_combo_data(self):
        """加载JSON数据并初始化干员列表"""
        try:
            with open(r".\resource\saves.json", "r", encoding="utf-8") as f:
                self.data = json.load(f)
            agents = list(self.data.keys())
            self.combo_agent.addItems(agents)
            self.combo_agent.setCurrentIndex(-1)
        except Exception as e:
            print(f"加载JSON数据失败: {e}")

    def load_brands(self):
        """加载品牌数据"""
        try:
            with open(r".\resource\brands.json", "r", encoding="utf-8") as f:
                self.brands = ["默认"] + list(json.load(f).keys())
            self.combo_skin.addItems(self.brands)
            self.combo_skin.setCurrentIndex(-1)
        except Exception as e:
            print(f"加载品牌数据失败: {e}")

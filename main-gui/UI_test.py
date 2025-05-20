# -*- coding: utf-8 -*-
import sys
import json
import os
from PyQt5 import QtWidgets
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QApplication, QWidget
from qfluentwidgets import (EditableComboBox, IconInfoBadge, FluentIcon, PrimaryPushButton, PrimaryToolButton,
                            TransparentToolButton, ToolButton, Flyout, FlyoutAnimationType, InfoBarIcon)


class AddCard(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("添加卡片 - 预览")
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
        self.verticalLayout.setContentsMargins(16, 8, 16, 8)

        # 水平布局（核心调整部分）
        self.horizontalLayout = QtWidgets.QHBoxLayout()

        # ================== 第一组（干员选择） ==================

        # 下拉框
        self.combo_agent = EditableComboBox()
        self.combo_agent.setPlaceholderText("选择干员")
        self.combo_agent.setFixedSize(135, 45)  # 固定尺寸
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
        self.btn_add.setIcon(FluentIcon.ADD)
        self.btn_add.setFixedSize(240, 45)
        self.btn_add.clicked.connect(self.on_add_clicked)

        # 清空按钮
        self.btn_clear = ToolButton()
        self.btn_clear.setIcon(FluentIcon.DELETE)
        self.btn_clear.setFixedSize(80, 40)
        self.btn_clear.clicked.connect(self.clear_selections)

        # 按钮布局
        btn_layout.addStretch()
        btn_layout.addWidget(self.btn_reload)
        btn_layout.addSpacing(15)
        btn_layout.addWidget(self.btn_add)
        btn_layout.addSpacing(15)
        btn_layout.addWidget(self.btn_clear)
        btn_layout.addStretch()

        # ================== 最终布局组装 ==================
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.verticalLayout.addStretch()
        self.verticalLayout.addWidget(btn_container)

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
        print(image_path)
        if not os.path.exists(image_path):
            self.show_error_flyout()
            return

        card_data = {
            "agent": agent,
            "skin": skin,
            "model": model,
            "image": image_path
        }

        print("添加卡片数据:", card_data)
        # self.ui.add_card(card_data)

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


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = AddCard()
    window.show()
    sys.exit(app.exec_())

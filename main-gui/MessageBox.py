from PyQt5.QtCore import Qt, QSize, pyqtSignal
from PyQt5.QtWidgets import QHBoxLayout, QVBoxLayout, QWidget, QCompleter

from qfluentwidgets import MessageBoxBase, SubtitleLabel, EditableComboBox, StrongBodyLabel, Flyout, InfoBarIcon


class CustomMessageBox(MessageBoxBase):
    """ 自定义信息收集对话框 """
    yesSignal = pyqtSignal()
    cancelSignal = pyqtSignal()

    def __init__(self, parent=None, agent_items=None, skin_items=None, brand_items=None, model_items=None):
        super().__init__(parent)

        self.agent_items = agent_items
        self.skin_items = skin_items
        self.brand_items = brand_items
        self.model_items = model_items

        self.setUpUI()
        self.initLayout()

    def setUpUI(self):
        # 主容器
        self.mainContainer = QWidget()
        self.mainLayout = QVBoxLayout(self.mainContainer)

        # 主标题
        self.titleLabel = SubtitleLabel("基础信息填写", self.mainContainer)
        self.titleLabel.setFixedSize(QSize(180, 30))
        self.titleLabel.setAlignment(Qt.AlignCenter)

        # 输入字段
        self.agentLabel, self.agentCombo = self.createInputGroup("干员代号", self.agent_items)
        self.skinLabel, self.skinCombo = self.createInputGroup("皮肤名称", self.skin_items)
        self.brandLabel, self.brandCombo = self.createInputGroup("品牌分类", self.brand_items)
        self.modelLabel, self.modelCombo = self.createInputGroup("模型分类", self.model_items)

    def initLayout(self):
        # 主布局
        self.viewLayout.addWidget(self.mainContainer)

        # 标题布局
        self.mainLayout.addWidget(self.titleLabel, 0, Qt.AlignCenter)
        self.mainLayout.addSpacing(30)

        # 输入字段布局
        self.mainLayout.addLayout(self.createInputRow(self.agentLabel, self.agentCombo))
        self.mainLayout.addSpacing(20)
        self.mainLayout.addLayout(self.createInputRow(self.skinLabel, self.skinCombo))
        self.mainLayout.addSpacing(20)
        self.mainLayout.addLayout(self.createInputRow(self.brandLabel, self.brandCombo))
        self.mainLayout.addSpacing(20)
        self.mainLayout.addLayout(self.createInputRow(self.modelLabel, self.modelCombo))

        # 尺寸设置
        self.widget.setMinimumSize(180, 180)

    def createInputGroup(self, title, items):
        """ 创建输入组元素 """
        label = StrongBodyLabel(title)
        label.setFixedSize(80, 40)
        combo = EditableComboBox()
        combo.setFixedSize(160, 40)

        combo.addItems(items)
        completer = QCompleter(items, combo)
        completer.setCaseSensitivity(Qt.CaseInsensitive)
        combo.setCompleter(completer)
        return label, combo

    def createInputRow(self, label, combo):
        """ 创建输入行布局 """
        layout = QHBoxLayout()
        layout.addStretch()
        layout.addWidget(label)
        layout.addWidget(combo)
        layout.addStretch()
        return layout

    def getInputs(self):
        """ 获取输入数据 """
        return {
            "干员代号": self.agentCombo.currentText(),
            "皮肤名称": self.skinCombo.currentText(),
            "品牌分类": self.brandCombo.currentText(),
            "模型分类": self.modelCombo.currentText()
        }

    def accept(self):
        flag = False

        if not self.agentCombo.currentText().strip():
            Flyout.create(icon=InfoBarIcon.ERROR, title='干员代号不能为空！', content="", target=self.agentCombo, parent=self)
            flag = True
        if not self.skinCombo.currentText().strip():
            Flyout.create(icon=InfoBarIcon.ERROR, title='皮肤名称不能为空！', content="", target=self.skinCombo, parent=self)
            flag = True
        if not self.brandCombo.currentText().strip():
            Flyout.create(icon=InfoBarIcon.ERROR, title='品牌分类不能为空！', content="", target=self.brandCombo, parent=self)
            flag = True
        if not self.modelCombo.currentText().strip():
            Flyout.create(icon=InfoBarIcon.ERROR, title='模型分类不能为空！', content="", target=self.modelCombo, parent=self)
            flag = True

        if not flag:
            super().accept()

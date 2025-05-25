from typing import Union
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QLabel
from qfluentwidgets import ConfigValidator, qconfig, Slider, FluentIconBase, SettingCard


class RangeValidator2F(ConfigValidator):
    """ 支持浮点数的范围验证器 """

    def __init__(self, min_val, max_val):
        self.min = float(min_val)
        self.max = float(max_val)
        self.range = (self.min, self.max)

    def validate(self, value):
        return self.min <= value <= self.max

    def correct(self, value):
        return max(self.min, min(float(value), self.max))


class RangeSettingCard2F(SettingCard):
    """ 支持两位小数的设置卡片 """
    valueChanged = pyqtSignal(float)  # 改为浮点信号

    def __init__(self, configItem, icon: Union[str, QIcon, FluentIconBase], title, content=None, scale=100, parent=None):
        super().__init__(icon, title, content, parent)
        self.scale = scale
        self.configItem = configItem

        # 初始化滑块
        self.slider = Slider(Qt.Horizontal, self)
        self.valueLabel = QLabel(self)

        # 设置滑块范围（放大100倍处理小数）
        min_val = int(configItem.range[0] * self.scale)
        max_val = int(configItem.range[1] * self.scale)
        self.slider.setRange(min_val, max_val)
        self.slider.setSingleStep(1)
        self.slider.setMaximumWidth(100)
        self.slider.setMinimumWidth(100)

        # 初始化显示值
        self._update_display(configItem.value)
        self.slider.setValue(int(configItem.value * self.scale))

        # 布局设置
        self.hBoxLayout.addStretch(1)
        self.hBoxLayout.addWidget(self.valueLabel, 0, Qt.AlignRight)
        self.hBoxLayout.addSpacing(6)
        self.hBoxLayout.addWidget(self.slider, 0, Qt.AlignRight)
        self.hBoxLayout.addSpacing(16)
        self.valueLabel.setObjectName('valueLabel')

        # 信号连接
        configItem.valueChanged.connect(self.setValue)
        self.slider.valueChanged.connect(self._on_slider_changed)

    def _on_slider_changed(self, slider_value: int):
        """ 处理滑块值变化 """
        actual_value = slider_value / self.scale
        self.setValue(actual_value)
        self.valueChanged.emit(actual_value)

    def setValue(self, value: float):
        """ 设置当前值 """
        # 验证并修正值
        corrected_value = self.configItem.validator.correct(float(value))

        # 更新配置系统
        qconfig.set(self.configItem, corrected_value)

        # 更新显示
        self._update_display(corrected_value)

        # 更新滑块位置（需要转换为整数）
        self.slider.blockSignals(True)
        self.slider.setValue(int(corrected_value * self.scale))
        self.slider.blockSignals(False)

    def _update_display(self, value: float):
        """ 更新显示文本 """
        self.valueLabel.setText(f"{value:.{2 if self.scale >= 10 else 1}f}")
        self.valueLabel.adjustSize()


class RangeSettingCardInt(SettingCard):
    """ Setting card with a slider """

    valueChanged = pyqtSignal(int)

    def __init__(self, configItem, icon: Union[str, QIcon, FluentIconBase], title, content=None, parent=None):
        super().__init__(icon, title, content, parent)
        self.configItem = configItem
        self.slider = Slider(Qt.Horizontal, self)
        self.valueLabel = QLabel(self)
        self.slider.setMaximumWidth(100)
        self.slider.setMinimumWidth(100)

        self.slider.setSingleStep(1)
        self.slider.setRange(*configItem.range)
        self.slider.setValue(configItem.value)
        self.valueLabel.setNum(configItem.value)

        self.hBoxLayout.addStretch(1)
        self.hBoxLayout.addWidget(self.valueLabel, 0, Qt.AlignRight)
        self.hBoxLayout.addSpacing(6)
        self.hBoxLayout.addWidget(self.slider, 0, Qt.AlignRight)
        self.hBoxLayout.addSpacing(16)

        self.valueLabel.setObjectName('valueLabel')
        configItem.valueChanged.connect(self.setValue)
        self.slider.valueChanged.connect(self.__onValueChanged)

    def __onValueChanged(self, value: int):
        """ slider value changed slot """
        self.setValue(value)
        self.valueChanged.emit(value)

    def setValue(self, value):
        qconfig.set(self.configItem, value)
        self.valueLabel.setText(str(value))
        self.valueLabel.adjustSize()
        self.slider.setValue(value)

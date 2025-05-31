from PyQt5.QtCore import QObject
from PyQt5.QtGui import QGuiApplication
from PyQt5.QtWidgets import QApplication


class DPIManager(QObject):
    def __init__(self, config):
        super().__init__()
        self.config = config

    def get_system_scale(self):
        """自动检测系统DPI缩放比例"""
        # 确保QApplication已经实例化
        app = QApplication.instance() or QApplication([])

        platform = QGuiApplication.platformName().lower()
        screen = app.primaryScreen()

        if platform in ['windows', 'xcb']:  # xcb对应Linux
            return screen.logicalDotsPerInchX() / 96.0
        elif platform == 'cocoa':  # macOS
            return screen.devicePixelRatio()
        else:
            return max(1.0, screen.physicalDotsPerInchX() / 96.0)

    def get_scale_factor(self):
        """获取最终使用的缩放比例"""
        config_value = self.config.get(self.config.dpiScale)

        if config_value == "Auto":
            system_scale = self.get_system_scale()
            # 对齐到最接近的预设值
            presets = [1, 1.25, 1.5, 1.75, 2]
            return min(presets, key=lambda x: abs(x - system_scale))
        else:
            return float(config_value)

    def scaled_value(self, base_value):
        """根据缩放比例计算实际值"""
        return int(base_value * self.get_scale_factor() * 2 / 3)

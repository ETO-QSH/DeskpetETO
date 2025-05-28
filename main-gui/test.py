import sys
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QApplication

from DeskpetETO.FileListSettingCard import FileListSettingCard
from DeskpetETO.FilesDropWidget import FilesDropWidget
from qfluentwidgets import ConfigItem


class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("文件拖放示例")
        self.resize(800, 600)

        # 初始化 FileListSettingCard
        self.file_list_card = FileListSettingCard(
            ConfigItem("", "", ""),
            title="选择模型文件",
            content="添加 Spine 动画所需的文件（*.skel *.atlas *.png）",
            directory="./"
        )
        self.file_list_card.stateFull.connect(self.on_state_full)
        self.file_list_card.sameName.connect(self.on_same_name)

        # 初始化 FilesDropWidget
        self.files_drop_widget = FilesDropWidget(file_types=[".png", ".skel", ".atlas"])
        self.files_drop_widget.bindFileListSettingCard(self.file_list_card)

        # 布局
        layout = QVBoxLayout()
        layout.addWidget(self.file_list_card)
        layout.addWidget(self.files_drop_widget)
        self.setLayout(layout)

    def on_state_full(self, is_full):
        print("文件列表状态:", "已满" if is_full else "未满")

    def on_same_name(self, is_full):
        print("文件列表状态:", "相同" if is_full else "不同")


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())

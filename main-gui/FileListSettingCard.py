import json
from pathlib import Path

from PyQt5.QtCore import Qt, pyqtSignal, QSize
from PyQt5.QtWidgets import QFileDialog, QFrame, QHBoxLayout, QLabel, QSizePolicy

from qfluentwidgets import ExpandSettingCard, ConfigItem, PushButton, FluentIcon, qconfig, ToolButton, FluentStyleSheet


class FileItem(QFrame):
    removed = pyqtSignal(object)

    def __init__(self, file_type: str, path: str, parent=None):
        super().__init__(parent=parent)
        self.file_type = file_type
        self.path = path

        self.titleLabel = QLabel(f"{file_type}:")
        self.titleLabel.setMaximumWidth(39)
        self.titleLabel.setMinimumWidth(39)
        self.titleLabel.setAlignment(Qt.AlignCenter)

        self.pathLabel = QLabel(Path(path).name)

        self.removeButton = ToolButton(FluentIcon.CLOSE, self)
        self.removeButton.setFixedSize(39, 29)
        self.removeButton.setIconSize(QSize(12, 12))
        self.removeButton.clicked.connect(lambda: self.removed.emit(self))

        self.hBoxLayout = QHBoxLayout(self)
        self.hBoxLayout.setContentsMargins(6, 0, 46, 0)
        self.hBoxLayout.addWidget(self.titleLabel, 0, Qt.AlignVCenter)
        self.hBoxLayout.addSpacing(5)
        self.hBoxLayout.addWidget(self.pathLabel, 0, Qt.AlignCenter)
        self.hBoxLayout.addStretch()
        self.hBoxLayout.addWidget(self.removeButton, 0, Qt.AlignCenter)
        self.hBoxLayout.setAlignment(Qt.AlignVCenter)

        self.setFixedHeight(53)
        self.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Fixed)
        FluentStyleSheet.SETTING_CARD.apply(self)


class FileListSettingCard(ExpandSettingCard):
    filesChanged = pyqtSignal(dict)
    stateFull = pyqtSignal(bool)

    def __init__(self, configItem: ConfigItem, title: str, content: str = None, directory="./", parent=None):
        super().__init__(FluentIcon.DOCUMENT, title, content, parent)
        self.configItem = configItem
        self._dialogDirectory = directory
        self.addButton = PushButton(self.tr('打开'), self)
        self.addButton.setMaximumWidth(80)
        self.addButton.setMinimumWidth(80)

        # 初始化文件字典
        config_value = qconfig.get(configItem)
        self.files = json.loads(config_value) if config_value else {'skel': '', 'atlas': '', 'png': ''}
        self.file_items = {}

        self.__initWidget()

    def __initWidget(self):
        self.card.expandButton.hide()
        self.addWidget(self.addButton)

        self.viewLayout.setSpacing(0)
        self.viewLayout.setAlignment(Qt.AlignTop)
        self.viewLayout.setContentsMargins(0, 0, 0, 0)

        # 初始化已存在的文件项
        for file_type in ['skel', 'atlas', 'png']:
            if self.files.get(file_type):
                self.__addFileItem(file_type, self.files[file_type])

        self.addButton.clicked.connect(self.__showFileDialog)
        self._checkState()

    def __showFileDialog(self):
        files, _ = QFileDialog.getOpenFileNames(
            self,
            self.tr("选择多个文件"),
            self._dialogDirectory,
            self.tr("Spine files (*.skel *.atlas *.png)")
        )

        if not files:
            return

        for path in files:
            ext = Path(path).suffix.lower()[1:]
            if ext in ['skel', 'atlas', 'png']:
                self.__updateFile(ext, path)

        qconfig.set(self.configItem, json.dumps(self.files))
        self.filesChanged.emit(self.files)
        self._checkState()

    def __updateFile(self, file_type: str, path: str):
        self.files[file_type] = path
        if file_type in self.file_items:
            self.file_items[file_type].path = path
            self.file_items[file_type].pathLabel.setText(Path(path).name)
        else:
            self.__addFileItem(file_type, path)

    def __addFileItem(self, file_type: str, path: str):
        item = FileItem(file_type, path, self.view)
        item.removed.connect(lambda: self.__removeFile(file_type))
        self.file_items[file_type] = item
        self.viewLayout.addWidget(item)
        item.show()
        self.setExpand(True)
        self._adjustViewSize()

    def __removeFile(self, file_type: str):
        if file_type not in self.file_items:
            return

        self.files[file_type] = ''
        item = self.file_items.pop(file_type)

        self.viewLayout.removeWidget(item)
        item.deleteLater()
        self._checkState()
        self._adjustViewSize()

        self.filesChanged.emit(self.files)
        qconfig.set(self.configItem, json.dumps(self.files))

    def _checkState(self):
        is_full = all(self.files.values())
        self.stateFull.emit(is_full)

import json
import copy
from pathlib import Path
from typing import Dict, Union, List

from DeskpetETO.Setting import cfg


class JsonMerger:
    def __init__(self, resource_dir="./resource"):
        self.resource_dir = Path(resource_dir)
        self.priority = cfg.wareHouse  # 存储优先级设置
        self._init_file_paths()

        # 主数据容器
        self.data = {}  # saves + user_saves
        self.brands = []  # 品牌列表
        self.brand_skin_dict = {}  # 品牌-皮肤映射

        # 初始化加载
        self.load_all_data()

    def _init_file_paths(self):
        """初始化文件路径"""
        self.file_paths = {
            "saves": self.resource_dir / "saves.json",
            "user_saves": self.resource_dir / "user_saves.json",
            "brands": self.resource_dir / "brands.json",
            "user_brands": self.resource_dir / "user_brands.json"
        }

    def load_all_data(self):
        """加载所有数据"""
        self.load_combo_data()
        self.load_brands()
        self.load_brands_data()

    def load_combo_data(self):
        """加载并合并干员数据"""
        main_data = self._load_json("saves")
        user_data = self._load_json("user_saves")
        self.data = self.merge(
            main_data=main_data,
            user_data=user_data,
            priority=self.priority
        )

    def load_brands(self):
        """加载品牌列表"""
        main_data = self._load_json("brands")
        user_data = self._load_json("user_brands")
        merged = self.merge(main_data, user_data, self.priority)
        self.brands = ["默认"] + list(merged.keys())

    def load_brands_data(self):
        """加载品牌-皮肤数据"""
        main_data = self._load_json("brands")
        user_data = self._load_json("user_brands")
        merged = self.merge(main_data, user_data, self.priority)
        self.brand_skin_dict = merged
        self.brand_skin_dict.setdefault("默认", ["默认"])

    def _load_json(self, file_type: str) -> Dict:
        """增强型加载方法"""
        try:
            with open(self.file_paths[file_type], "r", encoding="utf-8") as f:
                return json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            return {}
        except Exception as e:
            print(f"加载 {file_type} 失败: {str(e)}")
            return {}

    @staticmethod
    def merge(main_data: Dict, user_data: Dict, priority: str) -> Dict:
        """
        智能合并核心算法
        :param priority: "用户" 表示用户数据优先，否则仓库数据优先
        """
        def _deep_merge(primary: Dict, secondary: Dict) -> Dict:
            merged = copy.deepcopy(primary)
            for key, sec_value in secondary.items():
                pri_value = merged.get(key)
                if isinstance(pri_value, dict) and isinstance(sec_value, dict):
                    merged[key] = _deep_merge(pri_value, sec_value)
                else:
                    if key not in merged or priority == "用户":
                        merged[key] = copy.deepcopy(sec_value)
            return merged

        # 根据优先级决定主次数据
        if priority == "用户":
            return _deep_merge(user_data, main_data)
        return _deep_merge(main_data, user_data)

    def save_user_saves(self, new_data: Dict, overwrite: bool = False) -> Union[bool, List[str]]:
        """保存用户干员数据"""
        return self._save_user_data(
            file_type="user_saves",
            new_data=new_data,
            overwrite=overwrite
        )

    def save_user_brands(self, new_data: Dict, overwrite: bool = False) -> Union[bool, List[str]]:
        """保存用户品牌数据"""
        return self._save_user_data(
            file_type="user_brands",
            new_data=new_data,
            overwrite=overwrite
        )

    def _save_user_data(self, file_type: str, new_data: Dict, overwrite: bool) -> Union[bool, List[str]]:
        """增强型保存逻辑"""
        # 加载现有用户数据
        current_data = self._load_json(file_type)

        # 冲突检测
        conflict_paths = self._find_conflicts(current_data, new_data)

        if conflict_paths and not overwrite:
            return conflict_paths

        # 合并策略：强制覆盖时使用用户数据优先
        merged = self.merge(
            main_data=current_data,
            user_data=new_data,
            priority="用户" if overwrite else self.priority
        )

        # 保存更新
        self._save_json(file_type, merged)

        # 刷新数据
        self.load_all_data()
        return True

    def _find_conflicts(self, base: Dict, new: Dict, path: str = "") -> List[str]:
        """递归冲突检测"""
        conflicts = []
        for key, new_value in new.items():
            current_path = f"{path}.{key}" if path else key
            if key in base:
                if isinstance(new_value, dict) and isinstance(base[key], dict):
                    conflicts += self._find_conflicts(base[key], new_value, current_path)
                elif not isinstance(new_value, dict):
                    conflicts.append(current_path)
        return conflicts

    def _save_json(self, file_type: str, data: Dict):
        """带格式化的保存方法"""
        with open(self.file_paths[file_type], "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=4, sort_keys=True)

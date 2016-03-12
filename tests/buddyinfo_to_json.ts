interface MemoryNode {
	make_json() : any;
}

class HolderNode implements MemoryNode {
	children : Array<MemoryNode>;

	make_json() : any {
		var obj = {"children" : []};
		for (var c of this.children) {
			obj.children.push(c);
		}
		return obj
	}
}

class ContiguosNode implements MemoryNode {
	start : number
	end : number

	constructor(start: number, end: number) {
		this.start = start;
		this.end = end;
	}

	make_json() : any {
		return {"size": this.end - this.start}
	}
}

class BuddyMap {
	level_of: Array<number>;

	constructor(level_of: Array<number>) {
		this.level_of = level_of;
	}

	_is_range_in_one_level(start: number, end: number) : number {
		var l = this.level_of[start];
		for (var i = start; i < end; i++) {
			if (this.level_of[i] != l) {
				return -1;
			}
		}
		return l;
	}

	make_tree(start: number, end: number) : MemoryNode {
		var one_level = this._is_range_in_one_level(start, end);

		if (one_level != -1) {
			return new ContiguosNode(start, end);
		}


	}
};
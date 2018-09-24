import gdb
import re

def array_info_string(size, capacity, start, end):
    r = 'fo::Array with size = {}, cap = {}, start = {}, end = {}'.format(size, capacity, start, end)
    return r

class ScaffoldArrayIterator:
    def __init__(self, value):
        self.index = 0
        self.current = value['_data']
        self.end = value['_data'] + value['_size']
        self.size = value['_size']
        self.cap = value['_capacity']
        
    def __iter__(self):
        return self
        
    def __next__(self):
        if self.index == self.end:
            raise StopIteration()
            
        ret = ("[{}]".format(self.index), self.current.dereference())
        self.index += 1
        return ret
        

class ScaffoldArrayPrinter:
    def __init__(self, value):
        self.value = value
       
    def to_string(self):
        data = self.value['_data']
        end = data + self.value['_size']
        return array_info_string(self.value['_size'], self.value['_capacity'], data, end)
        
    def children(self):
        return ScaffoldArrayIterator(self.value)


array_regex = re.compile(r'^fo::Array<.*>$')        

def array_lookup_function(value):
    lookup_tag = value.type.tag
    
    if not lookup_tag:
        return None
        
    if re.match(array_regex, lookup_tag):
        return ScaffoldArrayPrinter(value)
        
    return None
    
def register_scaffold_printers(objfile):
    objfile.pretty_printers.append(array_lookup_function)


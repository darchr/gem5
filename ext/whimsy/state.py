class Result:
    enums = '''
        NotRun
        Skipped
        Passed
        Failed
        Errored
    '''.split()
    for idx, enum in enumerate(enums):
        locals()[enum] = idx

    @classmethod
    def name(cls, enum):
        return cls.enums[enum]

    def __init__(self, value, reason=None):
        self.value = value
        self.reason = reason
    
    def __str__(self):
        return self.name(self.value)

class Status:
    enums = '''
        Unscheduled
        Building
        Running
        TearingDown
        Complete
        Avoided
    '''.split()
    for idx, enum in enumerate(enums):
        locals()[enum] = idx

    @classmethod
    def name(cls, enum):
        return cls.enums[enum]
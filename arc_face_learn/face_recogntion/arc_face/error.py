class BaseException(Exception):

    def __init__(self, error_info):
        self.error = error_info

    def __str__(self):
        return self.error
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

####### Statements ########

class Statement:
    '''
    Base class for statement.
    '''
    def __init__(self):
        pass

    def __repr__(self):
        return '(statement)'

class ExprStatement:
    '''
    Expression statement. Any expression value is ignored.
    '''
    def __init__(self, expr):
        self.expr = expr

    def __repr__(self):
        return repr(self.expr)

class AssignmentStatement(Statement):
    '''
    Assignment statement.
    Assign rvalue to lvalue.
    '''
    def __init__(self, lvalue, rvalue):
        self.lvalue = lvalue
        self.rvalue = rvalue

    def __repr__(self):
        return '%s = %s' % (repr(self.lvalue), repr(self.rvalue))



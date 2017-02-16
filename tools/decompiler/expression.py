# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

####### Expressions ########

class Expression:
    '''Base class for expression node'''
    pass

class OpExpression(Expression):
    '''Operation'''
    def __init__(self, op, args):
        self.op = op
        self.args = args

    def __repr__(self):
        return '%s(%s)' % (self.op, ', '.join(repr(s) for s in self.args))

class ConstantExpression(Expression):
    '''Any kind of constant expression'''
    pass

class ConstantIntExpression(ConstantExpression):
    def __init__(self, val):
        self.val = val

    def __repr__(self):
        return '0x%x' % (self.val)

class NilExpression(ConstantExpression):
    def __init__(self):
        pass

    def __repr__(self):
        return 'nil'

class FunctionCall(Expression):
    '''Function call expression'''
    def __init__(self, func, sargs):
        self.func = func
        self.sargs = sargs

    def __repr__(self):
        return '%s(%s)' % (self.func, self.sargs)

class TakeAddressOf(Expression):
    '''Take address of expression'''
    def __init__(self, addrof):
        self.addrof = addrof

    def __repr__(self):
        return '&'+repr(self.addrof)

class VariableRef(Expression):
    '''Base class for variable references'''
    def __init__(self):
        pass

    def __repr__(self):
        return '(variableref)'

class GlobalVariableRef(VariableRef):
    '''Reference to a global variable'''
    def __init__(self, segment, num):
        self.segment = segment
        self.num = num

    def __repr__(self):
        return '%s_G%x' % (self.segment.rstrip().decode(),self.num)

class LocalVariableRef(VariableRef):
    '''Reference to a local variable of current function or encompassing functions'''
    def __init__(self, func, num):
        self.func = func
        self.num = num

    def __repr__(self):
        return '%s_%02x_L%x' % (self.func[0].rstrip().decode(), self.func[1], self.num)

class ParameterRef(LocalVariableRef):
    '''Reference to a parameter of current function or encompassing functions'''
    def __repr__(self):
        return '%s_%02x_P%x' % (self.func[0].rstrip().decode(), self.func[1], self.num)

class ReturnValueRef(LocalVariableRef):
    '''Reference to a return value of current function or encompassing functions'''
    def __repr__(self):
        return '%s_%02x_R%x' % (self.func[0].rstrip().decode(), self.func[1], self.num)


class TempVariableRef(VariableRef):
    '''Reference to a stack temporary'''
    def __init__(self, num):
        self.num = num

    def __repr__(self):
        return 'T%x' % (self.num)


from metadata import ProcedureMetaData, ProcedureList

def load_metadata():
    '''Build known procedure list for project'''
    import libcalls_list, appcalls_list 
    import libcalls_deltas, appcalls_deltas
    import libcalls_hierarchy, appcalls_hierarchy
    proclist = ProcedureList()
    proclist.load_deltas(libcalls_deltas.deltas)
    proclist.load_deltas(appcalls_deltas.deltas)
    proclist.load_hierarchy(libcalls_hierarchy.hierarchy)
    proclist.load_hierarchy(appcalls_hierarchy.hierarchy)
    proclist.load_proclist(libcalls_list.LIBCALLS)
    proclist.load_proclist(appcalls_list.APPCALLS)
    return proclist



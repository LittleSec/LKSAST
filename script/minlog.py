import sys
import re

astRe = r"\[!\] Handling AST: .*"
funRe = r"\[\+\] Analysis Function: .*"
asmRe1 = r"\[\!\] Warning, function body \[ .* \] has ASM Code!"
asmRe2 = r"  `-\[\!\] .*"
paramfunptrRe = r"\[PVD: '.*'\] isFunctionPointerType: .*"

astPat = re.compile(astRe)
funPat = re.compile(funRe)
asmPat1 = re.compile(asmRe1)
asmPat2 = re.compile(asmRe2)
paramfunptrPat = re.compile(paramfunptrRe)


def extractpvd(loglines):
    pvdloglines = []
    curAST = ""
    curASTisWrite = False
    for i in range(len(loglines)-1):
        if astPat.match(loglines[i]):
            curAST = loglines[i]
            curASTisWrite = False
            continue
        if paramfunptrPat.match(loglines[i]):
            if not curASTisWrite:
                pvdloglines.append(curAST)
                curASTisWrite = True
            pvdloglines.append(loglines[i])
            continue
    return pvdloglines


def extracterr(loglines):
    nopvdlog = []
    for l in loglines:
        if paramfunptrPat.match(l):
            continue
        nopvdlog.append(l)
    hasastloglines = []
    for i in range(len(nopvdlog)-1):
        if funPat.match(nopvdlog[i]) and funPat.match(nopvdlog[i+1]):
            continue
        if funPat.match(nopvdlog[i]) and astPat.match(nopvdlog[i+1]):
            continue
        hasastloglines.append(nopvdlog[i])
    hasastloglines.append(nopvdlog[-1])
    errloglines = []
    for i in range(len(hasastloglines)-1):
        if astPat.match(hasastloglines[i]) and astPat.match(hasastloglines[i+1]):
            continue
        errloglines.append(hasastloglines[i])
    errloglines.append(hasastloglines[-1])
    return errloglines


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Useage: python3 {0} path/to/log".format(sys.argv[0]))
        exit(1)
    else:
        logfilename = sys.argv[1]
        errlogfilename = logfilename + ".err"
        pvdlogfilename = logfilename + ".pvd"
        loglines = []
        with open(logfilename, mode='r') as fr:
            for l in fr.readlines():
                if l.strip() != "":
                    if asmPat1.match(l):
                        continue
                    if asmPat2.match(l):
                        continue
                    loglines.append(l)
        errloglines = extracterr(loglines)
        with open(errlogfilename, mode='w') as ferr:
            ferr.write("".join(errloglines))
        pvdloglines = extractpvd(loglines)
        with open(pvdlogfilename, mode='w') as fpvd:
            fpvd.write("".join(pvdloglines))

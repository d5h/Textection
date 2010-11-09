#!/usr/bin/env python

from __future__ import with_statement

import sys
import xml.dom.minidom as xmldom

def getNodeValue(node, name):
    children = node.getElementsByTagName(name)
    assert len(children) == 1 and children[0].firstChild.nodeType == node.TEXT_NODE
    return children[0].firstChild.data

def writeHeader():
    with open('classifier.h', 'w') as out:
        out.write('#ifndef CLASSIFIER_INCLUDED\n'
                  '#define CLASSIFIER_INCLUDED 1\n'
                  '#include "features.h"\n'
                  'int classify(objs_t &, size_t);\n'
                  '#endif //CLASSIFIER_INCLUDED\n')

def writeSource(weakhyp):
    with open('classifier.cc', 'w') as out:
        classes = weakhyp[0].getElementsByTagName('class')
        numClasses = len(classes)
        classValues = [c.getAttribute('id') for c in classes]
        classValueString = ', '.join(classValues)
        out.write('#include "classifier.h"\n')
        out.write('static int classes[%s] = {%s};\n' % (numClasses, classValueString))
        out.write('int\n'
                  'classify(objs_t &objs, size_t subject)\n'
                  '{\n'
                  '  AspectRatioFeature aspectRatio;\n'
                  '  TopPositionFeature topPosition;\n'
                  '  BottomPositionFeature bottomPosition;\n'
                  '  double valueAspectRatio = aspectRatio.describe(objs, subject);\n'
                  '  double valueTopPosition = topPosition.describe(objs, subject);\n'
                  '  double valueBottomPosition = bottomPosition.describe(objs, subject);\n')
        out.write('  double votes[%s] = {0};\n\n' % numClasses)
        for hyp in weakhyp:
            alpha = getNodeValue(hyp, 'alpha')
            thresh = getNodeValue(hyp, 'threshold')
            featureName = getNodeValue(hyp, 'column')
            votes = hyp.getElementsByTagName('class')
            for n, v in enumerate(votes):
                vVal = v.firstChild.data
                out.write('  votes[%s] += %s * (%s < value%s ? %s : -(%s));\n' %
                          (n, alpha, thresh, featureName, vVal, vVal))
        out.write('  double max = votes[0];\n'
                  '  size_t best = 0;\n'
                  '  for (size_t i = 0; i < %s; ++i)\n'
                  '    if (max < votes[i]) {\n'
                  '      max = votes[i];\n'
                  '      best = i;\n'
                  '    }\n'
                  '  return classes[best];\n'
                  '}\n' % numClasses)

def main(filename):
    writeHeader()
    dom = xmldom.parse(filename)
    weakhyp = dom.getElementsByTagName('weakhyp')
    writeSource(weakhyp)

if __name__ == '__main__':
    main(sys.argv[1])

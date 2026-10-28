
registry = {}

def register_pattern_callback(saveid, pattern):

    registry[saveid] = pattern
    commence_listening()

def rewrite(saveid, rle):

    x = registry[saveid] 
    newpat = x.owner.pattern(rle, x.getrule())
    x &= x.owner.pattern("", x.getrule())
    x += newpat

def commence_listening():

    from IPython.display import display, HTML

    display(HTML('''
    <div>
    <p style="color:green">Press <b>Ctrl+S</b> to save LifeViewer edits.</p>
    <script>
    var declaredListenerYet;
    if (!declaredListenerYet) {
        window.addEventListener('message',function(e) {
            var key = e.message ? 'message' : 'data';
            var data = e[key]
            console.log(data)
            if (data['rle']) {
                var rle = data['rle'].replace(/"/g, ''); // sanitise
                var saveid = data['uuid'].replace(/"/g, '');
                IPython.notebook.kernel.execute('import lifelib; lifelib.registry.rewrite("""' + saveid + '""", """' + rle + '""")');
            }
        },false);
        declaredListenerYet = true;
    }
    </script>
    </div>
    '''))


def identify_compression(filename):
    '''
    Determine the file compression algorithm by inspecting the first few bytes
    of the file. This supports bz2, gzip, and lzma compression.
    '''

    with open(filename, 'rb') as f:
        x = list(f.read(8))

    if isinstance(x[0], str):
        x = [ord(a) for a in x]

    if x[:3] == [0x42, 0x5a, 0x68]:
        return 'bz2'
    elif x[:2] == [0x1f, 0x8b]:
        return 'gzip'
    elif x[:7] == [0xfd, 0x37, 0x7a, 0x58, 0x5a, 0x00, 0x00]:
        return 'lzma'
    else:
        return None

def identify_metadata(filename):
    '''
    Identifies the file format and other details about a pattern file.
    '''

    metadata = {'format': 'rle', 'comments': []}

    with open(filename, 'r') as f:
        for l in f:
            l = l.strip()
            if l == '':
                continue
            if l[0] == '#':
                if (l[:8] == '#FRAMES ') and (metadata['format'] == 'macrocell'):
                    metadata['frames'] = l[8:].split()
                else:
                    metadata['comments'].append(l)
            elif l[0] == '[':
                metadata['format'] = 'macrocell'
            else:
                break

    return metadata

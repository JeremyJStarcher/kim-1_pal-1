fs = require('fs');

// These are the valid characters in the KIM-1 alphabet.
// Mapped to the KIM-1 LED pattern
const symbolMap = 
    Array.from(' abcdefghijlopsuynrt')
    .reduce((p, c ,i) => {
        p[c] = i
        return p;
    }, {});


const symbols = Object.keys(symbolMap);
Object.freeze(symbolMap);
Object.freeze(symbols);

const MIN_LENGTH = 4;
const MAX_LENGTH = 6;

function getValidWordList(words) {
    const validwords = [];

    words.forEach(word => {
        if (word.length < MIN_LENGTH || word.length > MAX_LENGTH) {
            return;
        }

        const letters = Array.from(word);

        // if the first character is a capitalized, assume it is a proper noun and drop it
        if (letters[0] == letters[0].toUpperCase()) {
            return;
        }

        for (let l of letters) {

            const isValid = symbols.indexOf(l.toLowerCase()) !== -1;
            if (!isValid) {
                return;
            }
        }

        validwords.push(word.toLowerCase());
    });

    return validwords;
}

// http://en.wikipedia.org/wiki/Fisher-Yates_shuffle#The_modern_algorithm
function shuffleArray(array) {
    const rnd = mulberry32(1000);

    for (let i = array.length - 1; i > 0; i--) {
        const j = Math.floor(rnd() * (i + 1));
        [array[i], array[j]] = [array[j], array[i]];
    }
}

function intToHex2(n) {
    let s = n.toString(16);
    if (s.length === 1) {
        s = '0' + s;
    }
    return `$${s}`;
}

function intToHex4(n) {
    let s = n.toString(16);
    while (s.length < 4) {
        s = '0' + s;
    }
    return `$${s}`;
}

function toBinary16(n) {
    let s = n.toString(2);
    while (s.length < 16) {
        s = '0' + s;
    }
    return `b${s}`;
}

function toMask16(n) {
    let maskbit = 0;
    let res = 0;

    for (i = 15; i > -1; i--) {
        const bit = 1 << i;
        // console.log(toBinary16(bit))
        if (n & bit) {
            maskbit = 1;
        }
        res = (res << 1) + maskbit
    }
    return res;
}


function getWordSet(wordlist, wordCount) {
    return wordlist.slice(0, wordCount);
}

function mulberry32(a) {
    return function () {
        var t = a += 0x6D2B79F5;
        t = Math.imul(t ^ t >>> 15, t | 1);
        t ^= t + Math.imul(t ^ t >>> 7, t | 61);
        return ((t ^ t >>> 14) >>> 0) / 4294967296;
    }
}

function generateAsm(wordSet, address) {
    const padding = Array(MAX_LENGTH * 2).fill(' ').join("");
    const b = [];

    wordSet.forEach((word, idx) => {
        const paddedWord = (word + padding).substring(0, MAX_LENGTH);
        const wordArray = Array.from(paddedWord);

        const hex = wordArray.map(char => {
            const charIntValue = symbolMap[char];
            return intToHex2(charIntValue);
        }).join(", ");

        const comment = paddedWord.replace(/ /g, "_");

        b.push(` .BYTE ${hex.toUpperCase()}; addr ${intToHex4(address)} word ${intToHex4(idx)} ${comment}`)
        address += wordArray.length;
    })

    return b;
}

const data = fs.readFileSync('/etc/dictionaries-common/words', { encoding: 'utf8', flag: 'r' });
const words = data.split(/\r\n|\r|\n/);

const validwords = getValidWordList(words);
shuffleArray(validwords);

// console.log(words.length, validwords.length);

const origin = 0x2000;
const ramTop = 0x3000;
const topPadding = 10;

const wordCount = Math.floor((ramTop - origin - topPadding) / MAX_LENGTH) - 1;
const mask = toMask16(wordCount);

let asm = [];

asm.push(`; NUMBER_OF_WORDS=${wordCount}  ${intToHex4(wordCount)}`);
asm.push(`          .org    ${intToHex4(origin)}`);

for (let i = 0; i < topPadding; i++) {
    switch (i) {
        case 0:
            asm.push(' ; Index of last word');
            asm.push(` .word ${intToHex4(wordCount-1)}  ; ${toBinary16(wordCount-1)}`);
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            asm.push(' ; The mask to help limit the random values.');
            asm.push(` .word ${intToHex4(mask)}  ; ${toBinary16(mask)}`);
            break;

        default:
            asm.push(` .byte $00`)
    }
}

const wordSet = getWordSet(validwords, wordCount);
const setAsm = generateAsm(wordSet, origin + topPadding);

asm = [...asm, ...setAsm];



fs.writeFileSync("wordlist.s",
    asm.join("\r\n"),
    {
        encoding: "utf8",
        flag: "w",
        mode: 0o644
    }
);

console.log("FILE WRITTEN");
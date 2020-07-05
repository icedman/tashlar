const clr = require('./colors.json');

clr.forEach(c => {   
    console.log(`{ ${c.rgb.r}, ${c.rgb.g}, ${c.rgb.b} }, // ${c.name}`)
})


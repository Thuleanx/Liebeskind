const Discord = require('discord.js');
const {prefix} = require('./appconfig.js');
const fs = require('fs');
require('dotenv').config();

const client = new Discord.Client();
client.commands = new Discord.Collection();
const cooldowns = new Discord.Collection();

const commandFiles = fs.readdirSync('./commands').filter(file => file.endsWith('.js'));
for (const file of commandFiles) {
	const command = require(`./commands/${file}`);
	client.commands.set(command.name, command);
}

client.on('ready', () => {
	console.log("Connected as " + client.user.tag);
	// List servers the bot is connected to
    console.log("Servers:");
    client.guilds.cache.forEach((guild) => {
        console.log(" - " + guild.name);
    })
});

client.on('message', (message) => {
	if (message.content.startsWith(prefix)) {
		let fullCmd = message.content.substr(1); // assumes prefix is one letter
		let splitCmd = fullCmd.split(" ");
		let cmdName = splitCmd[0];
		let args = splitCmd.splice(1);

		var command = client.commands.get(cmdName) || 
			client.commands.find(cmd => cmd.aliases && cmd.aliases.includes(cmdName));
		if (!command) return;

		// signal the start of the operation
		message.react('⚙️');

		if (command.guildOnly && message.channel.type === 'dm') {
			return message.reply('I can\'t execute that command inside DMs!');
		}

		if (command.dmOnly && message.channel.type !== 'dm') {
			return message.reply('I can\'t execute that command inside a server. Slide into my DM!');
		}

		if (command.args && args.length < command.args) {
			var msg = `You didn't provide the proper amount of arguments, ${message.author}!`;
			if (command.usage)
				msg += `\nThe proper usage would be: \`${prefix}${command.name} ${command.usage}\``;
			return message.channel.send(msg);
		}

		if (command.adminOnly && !message.member.hasPermission('ADMINISTRATOR')) {
			var msg = `You need administrative permission to use this command, ${message.author}.`	
			return message.channel.send(msg);
		}

		if (!cooldowns.has(command.name)) {
			cooldowns.set(command.name, new Discord.Collection());
		}

		const now = Date.now();
		const timestamps = cooldowns.get(command.name);
		const cooldownAmount = (command.cooldown || 3) * 1000;

		if (timestamps.has(message.author.id)) {
			const expirationTime = timestamps.get(message.author.id) + cooldownAmount;
		
			if (now < expirationTime) {
				const timeLeft = (expirationTime - now) / 1000;
				return message.reply(`Please wait ${timeLeft.toFixed(1)} more second(s) before reusing the \`${command.name}\` command.`);
			}
		}

		try {
			command.execute(message, args);

			timestamps.set(message.author.id, now);
			setTimeout(() => timestamps.delete(message.author.id), cooldownAmount);
		} catch (err) {
			console.log(err);
			message.channel.send(`草. Something went wrong. Please contact bot writer and let them know they did a poor job.`);
		}
		// signal the end of the operation
		message.react('👌');
	}
});

client.login(process.env.BOT_TOKEN);
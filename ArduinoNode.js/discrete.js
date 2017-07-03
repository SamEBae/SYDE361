function DiscreteView() {
	this.upArrow = document.getElementById('up-arrow');
	this.downArrow = document.getElementById('down-arrow');
	this.goodArrow = document.getElementById('good-arrow');
	// init by hiding the arrows
	this.hideAll();
}

DiscreteView.prototype.hideAll = function() {
	this.upArrow.style.display = 'none';
	this.downArrow.style.display = 'none';
	this.goodArrow.style.display = 'none';
};

DiscreteView.prototype.showUp = function() {
	this.upArrow.style.display = '';
};

DiscreteView.prototype.showDown = function() {
	this.downArrow.style.display = '';
};

DiscreteView.prototype.showGood = function() {
	this.goodArrow.style.display = '';
};

DiscreteView.prototype.handlePitch = function(pitch) {
	// pitch: {actual: float, desired: float}

	// hide before doing anything first:
	this.hideAll();


	// if pitch is bigger, then make arrow go down
	// if pitch is smaller, then make arrow go up
	// if pitch within acceptable range, display logo to indicate it's oaky
	var marginOfError = pitch.actual * 0.075,
		difference = Math.abs(pitch.actual -  pitch.desired);

	if (difference <= marginOfError) {
		this.showGood();
	} else if (pitch.actual < pitch.desired) {
		this.showUp();
	} else {
		this.showDown();
	}
};


// Tests
var view2 = new DiscreteView();
var pitch = {actual: 1000, desired: 950};
var pitch2 = {actual: 1000, desired: 920};
var pitch3 = {actual: 920, desired: 920};
var pitch4 = {actual: 820, desired: 920};